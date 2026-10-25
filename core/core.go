package core

import (
	"esp32/command"
	"esp32/led"
	"esp32/system"
	"fmt"
	"machine"
	"time"

	esplink "tinygo.org/x/espradio/netlink"
)

const (
	maxLineLength = 128
)

var (
	link = &esplink.Esplink{}
)

func Init() {
	led.InitLED()

	fmt.Println("\nCernel is running")

	var line [maxLineLength]byte
	length := 0

	for {
		if system.StartReboot {
			break
		}

		if machine.Serial.Buffered() == 0 {
			time.Sleep(5 * time.Millisecond)
			continue
		}

		c, err := machine.Serial.ReadByte()
		if err != nil {
			continue
		}

		switch c {
		case '\r', '\n':
			fmt.Println()

			if length > 0 {
				command.ExecuteCommand(link, string(line[:length]))
				length = 0
			}

		case 8, 127: // Backspace
			if length > 0 {
				length--
				fmt.Print("\b \b")
			}

		case 3: // Ctrl + C
			length = 0

		default:
			if c >= 32 && c < 127 && length < len(line) {
				line[length] = c
				length++
				machine.Serial.WriteByte(c)
			}
		}
	}
}
