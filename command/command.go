package command

import (
	"esp32/led"
	"esp32/wifi"
	"fmt"
	"strings"

	esplink "tinygo.org/x/espradio/netlink"
)

func ExecuteCommand(link *esplink.Esplink, input string) {
	parts := strings.Fields(input)
	if len(parts) == 0 {
		return
	}

	command := parts[0]
	args := parts[1:]

	switch command {
	case "help":
		fmt.Println("Available commands:")
		fmt.Println("	- help: you are here.")
		fmt.Println("	- print <text>: print text.")
		fmt.Println("	- led on/off: control built-in LED")
		fmt.Println("	- version: show version.")
		fmt.Println("	- wifi: run WiFi-related commands.")

	case "print":
		fmt.Println(strings.Join(args, " "))

	case "led":
		led.HandleLED(args)

	case "version":
		fmt.Println("Cernel v0.0.1")

	case "wifi":
		wifi.HandleWiFi(link, args)

	default:
		fmt.Println("Unknown command:", command)
		fmt.Println("Type\"help\" for available commands.")
	}
}
