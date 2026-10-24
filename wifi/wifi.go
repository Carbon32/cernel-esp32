package wifi

import (
	"fmt"

	nl "tinygo.org/x/drivers/netlink"
	esplink "tinygo.org/x/espradio/netlink"
)

type WiFiConfig struct {
	wifiSSID     string
	wifiPassword string
}

var defaultConfig = NewWiFiConfig("", "")

func NewWiFiConfig(SSID string, password string) *WiFiConfig {
	return &WiFiConfig{
		wifiSSID:     SSID,
		wifiPassword: password,
	}
}

func HandleWiFi(link *esplink.Esplink, args []string) {
	if len(args) < 1 {
		fmt.Println("Usage: wifi <show/connect/set>")
		return
	}

	switch args[0] {
	case "show":
		if len(args) != 1 {
			fmt.Println("Usage: wifi show takes no arguments")
			return
		}
		getWiFiInformation(link)

	case "connect":
		if len(args) != 1 {
			fmt.Println("Usage: wifi connect takes no arguments")
			return
		}
		connectWiFi(link, defaultConfig)

	case "set":
		if len(args) != 3 {
			fmt.Println("Usage: wifi set <SSID> <password>")
			return
		}

		defaultConfig = NewWiFiConfig(args[1], args[2])
		fmt.Println("WiFi set to:", args[1])

	default:
		fmt.Println("Usage: wifi <show/connect/set>")
	}
}

func connectWiFi(link *esplink.Esplink, config *WiFiConfig) {
	err := link.NetConnect(&nl.ConnectParams{
		Ssid:       config.wifiSSID,
		Passphrase: config.wifiPassword,
	})

	if err != nil {
		fmt.Println("Wi-Fi connection failed:", err)
		return
	}

	address, err := link.Addr()
	if err != nil {
		fmt.Println("Connected, but couldn't get IP address:", err)
		return
	}

	fmt.Println("Wi-Fi connected. IP set to:", address.String())
}

func getWiFiInformation(link *esplink.Esplink) {
	address, err := link.Addr()
	if err != nil {
		fmt.Println("No Wi-Fi address available:", err)
		return
	}

	if address.String() == "0.0.0.0" {
		fmt.Println("Not connected to any network")
	} else {
		fmt.Println("Wi-Fi IP:", address.String())
	}

}
