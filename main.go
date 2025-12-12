package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os/exec"
	"strconv"
	"strings"
	"time"

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

	nvidiaSmiAvailable bool = false

	gpuSamples      []float64
	gpuSampleWindow = 10
)

// ------------------ MAIN ------------------
func main() {
	// ------------------ NVIDIA-SMI CHECK ------------------
	if checkNvidiaSmi() {
		log.Println("nvidia-smi found, GPU monitoring enabled")
		nvidiaSmiAvailable = true
	} else {
		log.Println("nvidia-smi not found, GPU stats disabled")
		nvidiaSmiAvailable = false
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

// ------------------ GPU USAGE (nvidia-smi) ------------------
func checkNvidiaSmi() bool {
	cmd := exec.Command("nvidia-smi", "--version")
	err := cmd.Run()
	return err == nil
}

func sampleGPUUsage() {
	if !nvidiaSmiAvailable {
		return
	}

	// Query GPU utilization using nvidia-smi
	cmd := exec.Command("nvidia-smi", "--query-gpu=utilization.gpu", "--format=csv,noheader,nounits")
	output, err := cmd.Output()
	if err != nil {
		return
	}

	// Parse the output (should be a number like "45")
	gpuStr := strings.TrimSpace(string(output))
	gpuUsage, err := strconv.ParseFloat(gpuStr, 64)
	if err != nil {
		return
	}

	addGPUSample(gpuUsage)
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
