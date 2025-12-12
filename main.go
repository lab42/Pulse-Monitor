package main

import (
	"encoding/json"
	"fmt"
	"log"
	"strings"
	"time"

	"github.com/NVIDIA/go-nvml/pkg/nvml"
	"github.com/shirou/gopsutil/v3/cpu"
	"github.com/shirou/gopsutil/v3/mem"
	"github.com/shirou/gopsutil/v3/net"
	"go.bug.st/serial"
)

type SystemStats struct {
	CPU      float64 `json:"cpu"`
	Memory   float64 `json:"memory"`
	GPU      float64 `json:"gpu"`
	Upload   float64 `json:"upload"`
	Download float64 `json:"download"`
}

// ------------------ CONFIG ------------------
const esp32ID = "ID:91d8141364e544e181fca2382cd6751a"

// ------------------ GLOBALS ------------------
var (
	lastNetStats net.IOCountersStat
	lastTime     time.Time

	nvmlAvailable bool = false

	gpuSamples      []float64
	gpuSampleWindow = 10
)

// ------------------ MAIN ------------------
func main() {
	// ------------------ NVML INIT ------------------
	ret := nvml.Init()
	if ret != nvml.SUCCESS {
		log.Printf("NVML init failed (%s). GPU stats disabled.", nvml.ErrorString(ret))
		nvmlAvailable = false
	} else {
		nvmlAvailable = true
		defer nvml.Shutdown()
	}

	// ------------------ NETWORK INIT ------------------
	netStats, _ := net.IOCounters(false)
	if len(netStats) > 0 {
		lastNetStats = netStats[0]
		lastTime = time.Now()
	}

	// ------------------ ESP32 SERIAL INIT ------------------
	port := openPortWithRetry()
	defer port.Close()

	// ------------------ GPU SAMPLER ------------------
	go func() {
		ticker := time.NewTicker(200 * time.Millisecond)
		defer ticker.Stop()

		for range ticker.C {
			sampleGPUUsage()
		}
	}()

	// ------------------ MAIN LOOP ------------------
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		stats := getSystemStats()
		jsonData, err := json.Marshal(stats)
		if err != nil {
			log.Println("Error marshaling JSON:", err)
			continue
		}

		// Send JSON followed by newline
		_, err = port.Write(append(jsonData, '\n'))
		if err != nil {
			fmt.Println("Lost connection to Pulse Monitor — reconnecting...")
			port.Close()
			port = openPortWithRetry()
			continue
		}
	}
}

// ------------------ SYSTEM STATS ------------------
func getSystemStats() SystemStats {
	stats := SystemStats{}

	// CPU
	cpuPercent, err := cpu.Percent(0, false)
	if err == nil && len(cpuPercent) > 0 {
		stats.CPU = cpuPercent[0]
	}

	// Memory
	memInfo, err := mem.VirtualMemory()
	if err == nil {
		stats.Memory = memInfo.UsedPercent
	}

	// GPU
	stats.GPU = getSmoothedGPUUsage()

	// Network
	upload, download := getNetworkSpeed()
	stats.Upload = upload
	stats.Download = download

	return stats
}

// ------------------ NETWORK SPEED ------------------
func getNetworkSpeed() (float64, float64) {
	netStats, err := net.IOCounters(false)
	if err != nil || len(netStats) == 0 {
		return 0, 0
	}

	currentStats := netStats[0]
	currentTime := time.Now()

	timeDiff := currentTime.Sub(lastTime).Seconds()
	if timeDiff == 0 {
		return 0, 0
	}

	uploadBytes := float64(currentStats.BytesSent - lastNetStats.BytesSent)
	downloadBytes := float64(currentStats.BytesRecv - lastNetStats.BytesRecv)

	uploadMbps := (uploadBytes / timeDiff) * 8 / 1_000_000
	downloadMbps := (downloadBytes / timeDiff) * 8 / 1_000_000

	lastNetStats = currentStats
	lastTime = currentTime

	return uploadMbps, downloadMbps
}

// ------------------ GPU USAGE ------------------
func sampleGPUUsage() {
	if !nvmlAvailable {
		return
	}

	deviceCount, ret := nvml.DeviceGetCount()
	if ret != nvml.SUCCESS || deviceCount == 0 {
		return
	}

	dev, ret := nvml.DeviceGetHandleByIndex(0)
	if ret != nvml.SUCCESS {
		return
	}

	util, ret := dev.GetUtilizationRates()
	if ret != nvml.SUCCESS {
		return
	}

	addGPUSample(float64(util.Gpu))
}

func addGPUSample(sample float64) {
	gpuSamples = append(gpuSamples, sample)
	if len(gpuSamples) > gpuSampleWindow {
		gpuSamples = gpuSamples[1:]
	}
}

func getSmoothedGPUUsage() float64 {
	if len(gpuSamples) == 0 {
		return 0
	}
	var sum float64
	for _, v := range gpuSamples {
		sum += v
	}
	return sum / float64(len(gpuSamples))
}

// ------------------ ESP32 HANDSHAKE ------------------
func findESP32Port() (string, error) {
	ports, err := serial.GetPortsList()
	if err != nil {
		return "", err
	}

	for _, portName := range ports {
		mode := &serial.Mode{BaudRate: 115200}
		port, err := serial.Open(portName, mode)
		if err != nil {
			continue
		}

		// handshake using updated ID
		port.Write([]byte("ID:ed1d2a7c8af14a27b77b1c127d806aed\n"))
		buf := make([]byte, 128)

		port.SetReadTimeout(500 * time.Millisecond)
		n, _ := port.Read(buf)
		port.Close()

		if n > 0 && strings.Contains(string(buf[:n]), esp32ID) {
			return portName, nil
		}
	}

	return "", fmt.Errorf("no Pulse Monitor device found")
}

func openPortWithRetry() serial.Port {
	for {
		portName, err := findESP32Port()
		if err == nil {
			port, err := serial.Open(portName, &serial.Mode{BaudRate: 115200})
			if err == nil {
				fmt.Println("Connected to", portName)
				return port
			}
		}

		fmt.Println("Pulse Monitor not detected, retrying in 2 seconds...")
		time.Sleep(2 * time.Second)
	}
}
