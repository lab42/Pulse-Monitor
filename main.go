package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os/exec"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/shirou/gopsutil/v3/cpu"
	"github.com/shirou/gopsutil/v3/disk"
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
	Disk     float64 `json:"disk"`
}

// ------------------ CONFIG ------------------
const esp32ID = "ID:91d8141364e544e181fca2382cd6751a"
const sampleWindow = 5

// ------------------ GLOBALS ------------------
var (
	lastNetStats net.IOCountersStat
	lastNetTime  time.Time
	netMutex     sync.Mutex

	nvidiaSmiAvailable bool = false

	// Sample arrays with mutexes for thread safety
	cpuSamples      []float64
	cpuMutex        sync.Mutex
	memorySamples   []float64
	memoryMutex     sync.Mutex
	gpuSamples      []float64
	gpuMutex        sync.Mutex
	uploadSamples   []float64
	uploadMutex     sync.Mutex
	downloadSamples []float64
	downloadMutex   sync.Mutex
	diskSamples     []float64
	diskMutex       sync.Mutex
)

// ------------------ MAIN ------------------
func main() {
	if checkNvidiaSmi() {
		log.Println("nvidia-smi found, GPU monitoring enabled")
		nvidiaSmiAvailable = true
	} else {
		log.Println("nvidia-smi not found, GPU stats disabled")
		nvidiaSmiAvailable = false
	}

	netStats, _ := net.IOCounters(false)
	if len(netStats) > 0 {
		netMutex.Lock()
		lastNetStats = netStats[0]
		lastNetTime = time.Now()
		netMutex.Unlock()
	}

	port := openPortWithRetry()
	defer port.Close()

	// Start parallel samplers
	go cpuSampler()
	go memorySampler()
	go gpuSampler()
	go networkSampler()
	go diskSampler()

	// Send smoothed stats every second
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		stats := getSystemStats()
		jsonData, err := json.Marshal(stats)
		if err != nil {
			log.Println("Error marshaling JSON:", err)
			continue
		}

		log.Println(string(jsonData))

		_, err = port.Write(append(jsonData, '\n'))
		if err != nil {
			fmt.Println("Lost connection to Pulse Monitor — reconnecting...")
			port.Close()
			port = openPortWithRetry()
			continue
		}
	}
}

// ------------------ ROUNDING HELPER ------------------
func round2(v float64) float64 {
	return float64(int(v*100)) / 100
}

// ------------------ PARALLEL SAMPLERS ------------------
func cpuSampler() {
	ticker := time.NewTicker(200 * time.Millisecond)
	defer ticker.Stop()
	for range ticker.C {
		cpuPercent, err := cpu.Percent(0, false)
		if err == nil && len(cpuPercent) > 0 {
			cpuMutex.Lock()
			addSample(&cpuSamples, cpuPercent[0])
			cpuMutex.Unlock()
		}
	}
}

func memorySampler() {
	ticker := time.NewTicker(200 * time.Millisecond)
	defer ticker.Stop()
	for range ticker.C {
		memInfo, err := mem.VirtualMemory()
		if err == nil {
			memoryMutex.Lock()
			addSample(&memorySamples, memInfo.UsedPercent)
			memoryMutex.Unlock()
		}
	}
}

func gpuSampler() {
	ticker := time.NewTicker(200 * time.Millisecond)
	defer ticker.Stop()
	for range ticker.C {
		if !nvidiaSmiAvailable {
			continue
		}
		cmd := exec.Command("nvidia-smi", "--query-gpu=utilization.gpu", "--format=csv,noheader,nounits")
		output, err := cmd.Output()
		if err != nil {
			continue
		}
		gpuUsage, err := strconv.ParseFloat(strings.TrimSpace(string(output)), 64)
		if err != nil {
			continue
		}
		gpuMutex.Lock()
		addSample(&gpuSamples, gpuUsage)
		gpuMutex.Unlock()
	}
}

func networkSampler() {
	ticker := time.NewTicker(200 * time.Millisecond)
	defer ticker.Stop()
	for range ticker.C {
		netStats, err := net.IOCounters(false)
		if err != nil || len(netStats) == 0 {
			continue
		}

		currentStats := netStats[0]
		currentTime := time.Now()

		netMutex.Lock()
		timeDiff := currentTime.Sub(lastNetTime).Seconds()
		if timeDiff == 0 {
			netMutex.Unlock()
			continue
		}

		// Calculate Mbps
		uploadMbps := float64(currentStats.BytesSent-lastNetStats.BytesSent) * 8 / timeDiff / 1_000_000
		downloadMbps := float64(currentStats.BytesRecv-lastNetStats.BytesRecv) * 8 / timeDiff / 1_000_000

		// Sanity checks
		if uploadMbps < 0 {
			uploadMbps = 0
		}
		if downloadMbps < 0 {
			downloadMbps = 0
		}
		const maxMbps = 9999.99
		if uploadMbps > maxMbps {
			uploadMbps = maxMbps
		}
		if downloadMbps > maxMbps {
			downloadMbps = maxMbps
		}

		lastNetStats = currentStats
		lastNetTime = currentTime
		netMutex.Unlock()

		uploadMutex.Lock()
		addSample(&uploadSamples, uploadMbps)
		uploadMutex.Unlock()

		downloadMutex.Lock()
		addSample(&downloadSamples, downloadMbps)
		downloadMutex.Unlock()
	}
}

func diskSampler() {
	// Disk usage changes slowly, sample every 5 seconds
	ticker := time.NewTicker(5 * time.Second)
	defer ticker.Stop()
	for range ticker.C {
		partitions, err := disk.Partitions(false)
		if err != nil {
			continue
		}

		var totalSize, totalUsed uint64
		for _, partition := range partitions {
			usage, err := disk.Usage(partition.Mountpoint)
			if err != nil {
				continue
			}
			totalSize += usage.Total
			totalUsed += usage.Used
		}

		if totalSize == 0 {
			continue
		}
		diskPercent := float64(totalUsed) / float64(totalSize) * 100

		diskMutex.Lock()
		addSample(&diskSamples, diskPercent)
		diskMutex.Unlock()
	}
}

// ------------------ SAMPLE MANAGEMENT ------------------
func addSample(samples *[]float64, value float64) {
	*samples = append(*samples, value)
	if len(*samples) > sampleWindow {
		*samples = (*samples)[1:]
	}
}

func getSmoothed(samples []float64, mutex *sync.Mutex) float64 {
	mutex.Lock()
	defer mutex.Unlock()

	if len(samples) == 0 {
		return 0
	}
	var sum float64
	for _, v := range samples {
		sum += v
	}
	return sum / float64(len(samples))
}

// ------------------ SYSTEM STATS ------------------
func getSystemStats() SystemStats {
	return SystemStats{
		CPU:      round2(getSmoothed(cpuSamples, &cpuMutex)),
		Memory:   round2(getSmoothed(memorySamples, &memoryMutex)),
		GPU:      round2(getSmoothed(gpuSamples, &gpuMutex)),
		Upload:   round2(getSmoothed(uploadSamples, &uploadMutex)),
		Download: round2(getSmoothed(downloadSamples, &downloadMutex)),
		Disk:     round2(getSmoothed(diskSamples, &diskMutex)),
	}
}

// ------------------ GPU ------------------
func checkNvidiaSmi() bool {
	cmd := exec.Command("nvidia-smi", "--version")
	return cmd.Run() == nil
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
