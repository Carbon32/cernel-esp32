package main

import (
	"esp32/core"
)

/*
#define RTC_CNTL_OPTIONS0_REG 0x3ff48000
#define RTC_CNTL_SW_SYS_RST   (1 << 31)

static inline void direct_hardware_reset() {
    volatile unsigned int *reg = (unsigned int *)RTC_CNTL_OPTIONS0_REG;
    *reg |= RTC_CNTL_SW_SYS_RST;
}
*/
import "C"

func main() {
	core.Init()

	// I honestly couldn't figure out any other way to perform a restart so forcing a restart kind of worked
	C.direct_hardware_reset()
	for {
		// Leave this here, it makes the reboot goblin happy
	}
}
