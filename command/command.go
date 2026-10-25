package command

import (
	"esp32/led"
	"esp32/wifi"
	"fmt"
	"strings"

	esplink "tinygo.org/x/espradio/netlink"
)

var history = [5]string{"", "", "", "", ""}
var historyCounter = 0

func addEntry(input string) {
	history[historyCounter] = input
	historyCounter = (historyCounter + 1) % len(history)
}

func ExecuteCommand(link *esplink.Esplink, input string) {
	addEntry(input)

	parts := strings.Fields(input)
	if len(parts) == 0 {
		return
	}

	command := strings.ToLower(parts[0])
	args := parts[1:]

	switch command {
	case "help":
		fmt.Println("Available commands:")
		fmt.Println("	- help: you are here.")
		fmt.Println("	- print <text>: print text.")
		fmt.Println("	- history: show command history")
		fmt.Println("	- led on/off: control built-in LED")
		fmt.Println("	- version: show version.")
		fmt.Println("	- wifi: run WiFi-related commands.")

	case "print":
		var str string = strings.Join(args, " ")
		if str == "" {
			return
		}
		fmt.Println(strings.Join(args, " "))

	case "led":
		led.HandleLED(args)

	case "history":
		for _, value := range history {
			if value == "" {
				fmt.Println("- (empty)")
			} else {
				fmt.Println("- ", value)
			}
		}

	case "version":
		fmt.Println("Cernel v0.0.1")

	case "wifi":
		wifi.HandleWiFi(link, args)

	default:
		fmt.Println("Unknown command:", command)
		fmt.Println("Type\"help\" for available commands.")
	}
}
