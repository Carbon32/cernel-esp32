package led

import (
	"fmt"
	"machine"
)

func InitLED() {
	machine.LED.Configure(machine.PinConfig{Mode: machine.PinOutput})
}

func HandleLED(args []string) {
	if len(args) != 1 {
		fmt.Println("Usage: led <on/off>")
		return
	}

	switch args[0] {
	case "on":
		if machine.LED.Get() {
			fmt.Println("LED is already on")
			return
		}
		machine.LED.High()
		fmt.Println("LED turned on")

	case "off":
		if !machine.LED.Get() {
			fmt.Println("LED is already off")
			return
		}
		machine.LED.Low()
		fmt.Println("LED turned off")

	default:
		fmt.Println("Usage: led <on/off>")
	}
}
