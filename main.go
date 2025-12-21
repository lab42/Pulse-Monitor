package main

import (
	"fmt"
	"log"
	"os/exec"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/shamaton/msgpack/v2"
	"github.com/shirou/gopsutil/v3/cpu"
	"github.com/shirou/gopsutil/v3/disk"
	"github.com/shirou/gopsutil/v3/mem"
	"github.com/shirou/gopsutil/v3/net"
	"go.bug.st/serial"
)

// ------------------ CONFIG ------------------
const (
	esp32ID      = "ID:91d8141364e544e181fca2382cd6751a"
	hostID       = "ID:ed1d2a7c8af14a27b77b1c127d806aed"
	sampleWindow = 5
	baudRate     = 115200
)

// ------------------ METRIC TRACKER ------------------
type metricTracker struct {
	samples []float64
	mu      sync.Mutex
}

func (m *metricTracker) add(value float64) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.samples = append(m.samples, value)
	if len(m.samples) > sampleWindow {
		m.samples = m.samples[1:]
	}
}

func (m *metricTracker) average() float64 {
	m.mu.Lock()
	defer m.mu.Unlock()
	if len(m.samples) == 0 {
		return 0
	}
	var sum float64
	for _, v := range m.samples {
		sum += v
	}
	return sum / float64(len(m.samples))
}

// ------------------ GLOBALS ------------------
var (
	cpuTracker      metricTracker
	memoryTracker   metricTracker
	gpuTracker      metricTracker
	uploadTracker   metricTracker
	downloadTracker metricTracker
	diskTracker     metricTracker

	lastNetStats       net.IOCountersStat
	netMutex           sync.Mutex
	nvidiaSmiAvailable bool
)

// ------------------ MAIN ------------------
func main() {
	nvidiaSmiAvailable = checkNvidiaSmi()
	if nvidiaSmiAvailable {
		log.Println("nvidia-smi found, GPU monitoring enabled")
	} else {
		log.Println("nvidia-smi not found, GPU stats disabled")
	}

	initializeNetworkStats()

	port := openPortWithRetry()
	defer port.Close()

	startSamplers()
	sendStatsLoop(port)
}

func initializeNetworkStats() {
	if netStats, err := net.IOCounters(false); err == nil && len(netStats) > 0 {
		netMutex.Lock()
		lastNetStats = netStats[0]
		netMutex.Unlock()
	}
}

func startSamplers() {
	// Prime values so UI updates immediately
	sampleCPU()
	sampleMemory()
	sampleGPU()
	sampleNetwork()
	sampleDisk()

	go runSampler(200*time.Millisecond, sampleCPU)
	go runSampler(200*time.Millisecond, sampleMemory)
	go runSampler(200*time.Millisecond, sampleGPU)
	go runSampler(200*time.Millisecond, sampleNetwork)
	go runSampler(5*time.Second, sampleDisk)
}

// ------------------ SEND LOOP ------------------
func sendStatsLoop(port serial.Port) {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		stats := buildMsgPackMap()

		payload, err := msgpack.Marshal(stats)
		if err != nil {
			log.Println("MsgPack marshal error:", err)
			continue
		}

		// newline-framed binary MessagePack
		if _, err = port.Write(append(payload, '\n')); err != nil {
			fmt.Println("Lost connection to Pulse Monitor — reconnecting...")
			port.Close()
			port = openPortWithRetry()
		}
	}
}

// ------------------ SAMPLERS ------------------
func runSampler(interval time.Duration, sampleFunc func()) {
	ticker := time.NewTicker(interval)
	defer ticker.Stop()
	for range ticker.C {
		sampleFunc()
	}
}

func sampleCPU() {
	if cpuPercent, err := cpu.Percent(0, false); err == nil && len(cpuPercent) > 0 {
		cpuTracker.add(cpuPercent[0])
	}
}

func sampleMemory() {
	if memInfo, err := mem.VirtualMemory(); err == nil {
		memoryTracker.add(memInfo.UsedPercent)
	}
}

func sampleGPU() {
	if !nvidiaSmiAvailable {
		return
	}
	cmd := exec.Command(
		"nvidia-smi",
		"--query-gpu=utilization.gpu",
		"--format=csv,noheader,nounits",
	)
	if output, err := cmd.Output(); err == nil {
		if gpuUsage, err := strconv.ParseFloat(strings.TrimSpace(string(output)), 64); err == nil {
			gpuTracker.add(gpuUsage)
		}
	}
}

func sampleNetwork() {
	netStats, err := net.IOCounters(false)
	if err != nil || len(netStats) == 0 {
		return
	}

	current := netStats[0]
	const timeDiff = 0.2 // 200 ms

	netMutex.Lock()
	bytesSent := current.BytesSent - lastNetStats.BytesSent
	bytesRecv := current.BytesRecv - lastNetStats.BytesRecv
	lastNetStats = current
	netMutex.Unlock()

	uploadMbps := clamp(float64(bytesSent)*8/timeDiff/1_000_000, 0, 9999.99)
	downloadMbps := clamp(float64(bytesRecv)*8/timeDiff/1_000_000, 0, 9999.99)

	uploadTracker.add(uploadMbps)
	downloadTracker.add(downloadMbps)
}

func sampleDisk() {
	partitions, err := disk.Partitions(false)
	if err != nil {
		return
	}

	var totalSize, totalUsed uint64
	for _, p := range partitions {
		if usage, err := disk.Usage(p.Mountpoint); err == nil {
			totalSize += usage.Total
			totalUsed += usage.Used
		}
	}

	if totalSize > 0 {
		diskTracker.add(float64(totalUsed) / float64(totalSize) * 100)
	}
}

// ------------------ MSGPACK PAYLOAD ------------------
func buildMsgPackMap() map[string]float64 {
	return map[string]float64{
		"cpu":      round2(cpuTracker.average()),
		"memory":   round2(memoryTracker.average()),
		"gpu":      round2(gpuTracker.average()),
		"upload":   round2(uploadTracker.average()),
		"download": round2(downloadTracker.average()),
		"disk":     round2(diskTracker.average()),
	}
}

// ------------------ HELPERS ------------------
func clamp(value, min, max float64) float64 {
	if value < min {
		return min
	}
	if value > max {
		return max
	}
	return value
}

func round2(v float64) float64 {
	return float64(int(v*100)) / 100
}

func checkNvidiaSmi() bool {
	cmd := exec.Command("nvidia-smi", "--version")
	return cmd.Run() == nil
}

// ------------------ ESP32 CONNECTION ------------------
func findESP32Port() (string, error) {
	ports, err := serial.GetPortsList()
	if err != nil {
		return "", err
	}

	for _, portName := range ports {
		if isESP32Port(portName) {
			return portName, nil
		}
	}
	return "", fmt.Errorf("no Pulse Monitor device found")
}

func isESP32Port(portName string) bool {
	port, err := serial.Open(portName, &serial.Mode{BaudRate: baudRate})
	if err != nil {
		return false
	}
	defer port.Close()

	port.Write([]byte(hostID + "\n"))
	buf := make([]byte, 128)
	port.SetReadTimeout(500 * time.Millisecond)
	n, _ := port.Read(buf)

	return n > 0 && strings.Contains(string(buf[:n]), esp32ID)
}

func openPortWithRetry() serial.Port {
	for {
		portName, err := findESP32Port()
		if err == nil {
			if port, err := serial.Open(portName, &serial.Mode{BaudRate: baudRate}); err == nil {
				fmt.Println("Connected to", portName)
				return port
			}
		}
		fmt.Println("Pulse Monitor not detected, retrying in 2 seconds...")
		time.Sleep(2 * time.Second)
	}
}
