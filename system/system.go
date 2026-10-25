package system

import (
	"fmt"
	"machine"
	"runtime"
	"time"
)

var systemStart time.Time = time.Now()
var StartReboot = false

func Uptime() {
	elapsed := time.Since(systemStart)
	total_seconds := int64(elapsed.Seconds())

	hours := total_seconds / 3600
	minutes := (total_seconds % 3600) / 60
	seconds := total_seconds % 60

	fmt.Printf("Uptime: %dh %dm %ds\n", hours, minutes, seconds)

}

func Mem() {
	var stats runtime.MemStats
	runtime.ReadMemStats(&stats)

	fmt.Printf("Heap allocated: %d bytes\n", stats.Alloc)
	fmt.Printf("Total allocated: %d bytes\n", stats.TotalAlloc)
	fmt.Printf("Runtime memory: %d bytes\n", stats.Sys)
}

func CPU() {
	hz := machine.CPUFrequency()
	fmt.Printf("CPU frequency: %d MHz\n", hz/1_000_000)
}

func Reboot() {
	StartReboot = true
}
