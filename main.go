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
	"github.com/shirou/gopsutil/v3/disk"
	"github.com/shirou/gopsutil/v3/mem"
	"github.com/shirou/gopsutil/v3/net"
	"go.bug.st/serial"
)

type SystemStats struct {
	CPU       float64 `json:"cpu"`
	Memory    float64 `json:"memory"`
	GPU       float64 `json:"gpu"`
	Upload    float64 `json:"upload"`
	Download  float64 `json:"download"`
	DiskUsage float64 `json:"disk_usage"`
}

// ------------------ CONFIG ------------------
const esp32ID = "ID:91d8141364e544e181fca2382cd6751a"

// ------------------ GLOBALS ------------------
var (
	lastNetStats net.IOCountersStat
	lastNetTime  time.Time

	nvidiaSmiAvailable bool = false

	gpuSamples      []float64
	gpuSampleWindow = 5

	uploadSamples       []float64
	downloadSamples     []float64
	networkSampleWindow = 5
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
		lastNetStats = netStats[0]
		lastNetTime = time.Now()
	}

	port := openPortWithRetry()
	defer port.Close()

	go func() {
		ticker := time.NewTicker(200 * time.Millisecond)
		defer ticker.Stop()
		for range ticker.C {
			sampleGPUUsage()
			sampleNetworkSpeed()
		}
	}()

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

// ------------------ SYSTEM STATS ------------------
func getSystemStats() SystemStats {
	stats := SystemStats{}

	cpuPercent, err := cpu.Percent(0, false)
	if err == nil && len(cpuPercent) > 0 {
		stats.CPU = round2(cpuPercent[0])
	} else {
		stats.CPU = 0
	}

	memInfo, err := mem.VirtualMemory()
	if err == nil {
		stats.Memory = round2(memInfo.UsedPercent)
	} else {
		stats.Memory = 0
	}

	stats.GPU = round2(getSmoothedGPUUsage())
	stats.Upload = round2(getSmoothedUpload())
	stats.Download = round2(getSmoothedDownload())
	stats.DiskUsage = round2(getDiskUsage())

	return stats
}

// ------------------ NETWORK SPEED ------------------
func sampleNetworkSpeed() {
	netStats, err := net.IOCounters(false)
	if err != nil || len(netStats) == 0 {
		return
	}

	currentStats := netStats[0]
	currentTime := time.Now()
	timeDiff := currentTime.Sub(lastNetTime).Seconds()
	if timeDiff == 0 {
		return
	}

	// Calculate Mbps (megabits per second)
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

	addUploadSample(uploadMbps)
	addDownloadSample(downloadMbps)

	lastNetStats = currentStats
	lastNetTime = currentTime
}

func addUploadSample(sample float64) {
	uploadSamples = append(uploadSamples, sample)
	if len(uploadSamples) > networkSampleWindow {
		uploadSamples = uploadSamples[1:]
	}
}

func addDownloadSample(sample float64) {
	downloadSamples = append(downloadSamples, sample)
	if len(downloadSamples) > networkSampleWindow {
		downloadSamples = downloadSamples[1:]
	}
}

func getSmoothedUpload() float64 {
	if len(uploadSamples) == 0 {
		return 0
	}
	var sum float64
	for _, v := range uploadSamples {
		sum += v
	}
	return sum / float64(len(uploadSamples))
}

func getSmoothedDownload() float64 {
	if len(downloadSamples) == 0 {
		return 0
	}
	var sum float64
	for _, v := range downloadSamples {
		sum += v
	}
	return sum / float64(len(downloadSamples))
}

// ------------------ DISK USAGE ------------------
func getDiskUsage() float64 {
	partitions, err := disk.Partitions(false)
	if err != nil {
		return 0
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
		return 0
	}
	return float64(totalUsed) / float64(totalSize) * 100
}

// ------------------ GPU ------------------
func checkNvidiaSmi() bool {
	cmd := exec.Command("nvidia-smi", "--version")
	return cmd.Run() == nil
}

func sampleGPUUsage() {
	if !nvidiaSmiAvailable {
		return
	}
	cmd := exec.Command("nvidia-smi", "--query-gpu=utilization.gpu", "--format=csv,noheader,nounits")
	output, err := cmd.Output()
	if err != nil {
		return
	}
	gpuUsage, err := strconv.ParseFloat(strings.TrimSpace(string(output)), 64)
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
