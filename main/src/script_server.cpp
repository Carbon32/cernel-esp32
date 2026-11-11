#include "core/bytecode.hpp"

#include <cerrno>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

extern "C" void shell_redraw_prompt(void);
extern "C" void shell_notify(const char *message);

namespace
{
constexpr char tag[] = "carblang_net";
constexpr EventBits_t connected_bit = BIT0;
constexpr uint16_t server_port = 3333;
constexpr size_t max_upload_size = 24 * 1024;
constexpr size_t max_header_size = 192;
constexpr char unavailable[] = "not connected";

EventGroupHandle_t wifi_events;

// These atomic booleans are thread-safe, didn't know they existed before
std::atomic_bool wifi_connected{false};
std::atomic_bool wifi_connecting{false};
std::atomic_bool wifi_started{false};
std::atomic_bool wifi_auto_reconnect{false};
std::atomic_bool wifi_reconfiguring{false};
char address_text[16] = "not connected";
char wifi_ssid[33]{};
char wifi_password[65]{};
bool wifi_stack_ready = false;
bool server_task_started = false;

bool valid_upload_token()
{
    return std::strcmp(CONFIG_CARBLANG_UPLOAD_TOKEN, "default_carblang_token") != 0 &&
           std::strlen(CONFIG_CARBLANG_UPLOAD_TOKEN) >= 16 &&
           std::strlen(CONFIG_CARBLANG_UPLOAD_TOKEN) <= 95;
}

bool init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(tag, "NVS initialization failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool constant_time_equal(const char *left, const char *right)
{
    const size_t left_size = std::strlen(left);
    const size_t right_size = std::strlen(right);
    if (left_size != right_size)
        return false;
    unsigned char difference = 0;
    for (size_t i = 0; i < left_size; ++i)
        difference |= static_cast<unsigned char>(left[i] ^ right[i]);
    return difference == 0;
}

void wifi_event_handler(void *, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        wifi_reconfiguring = false;
        if (wifi_auto_reconnect)
        {
            wifi_connecting = true;
            esp_wifi_connect();
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        wifi_connecting = false;
        std::strncpy(address_text, unavailable, sizeof(address_text) - 1);
        address_text[sizeof(address_text) - 1] = '\0';
        xEventGroupClearBits(wifi_events, connected_bit);
        if (wifi_started && wifi_auto_reconnect && !wifi_reconfiguring)
        {
            wifi_connecting = true;
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        const auto *event = static_cast<const ip_event_got_ip_t *>(event_data);
        std::snprintf(address_text, sizeof(address_text), IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        wifi_connecting = false;
        xEventGroupSetBits(wifi_events, connected_bit);
        char message[96];
        std::snprintf(message, sizeof(message),
            "Connected. Bytecode server at %s:%u",
            address_text, server_port
        );
        shell_notify(message);
    }
}

bool receive_exact(int socket, char *buffer, size_t size)
{
    size_t offset = 0;
    while (offset < size)
    {
        const int received = recv(socket, buffer + offset, size - offset, 0);
        if (received <= 0)
            return false;
        offset += static_cast<size_t>(received);
    }
    return true;
}

bool receive_header(int socket, char *header, size_t capacity)
{
    size_t length = 0;
    while (length + 1 < capacity)
    {
        char byte;
        const int received = recv(socket, &byte, 1, 0);
        if (received != 1)
            return false;
        if (byte == '\n')
        {
            header[length] = '\0';
            return true;
        }
        if (byte != '\r')
            header[length++] = byte;
    }
    return false;
}

void reply(int socket, const char *message)
{
    size_t sent = 0;
    const size_t length = std::strlen(message);
    while (sent < length)
    {
        const int count = send(socket, message + sent, length - sent, 0);
        if (count <= 0)
            return;
        sent += static_cast<size_t>(count);
    }
}

void handle_client(int client)
{
    char header[max_header_size];
    if (!receive_header(client, header, sizeof(header)))
    {
        reply(client, "ERR invalid header\n");
        return;
    }

    char protocol[16]{};
    char supplied_token[96]{};
    unsigned long payload_size = 0;
    char trailing = '\0';
    if (std::sscanf(header, "%15s %95s %lu %c", protocol, supplied_token,
                     &payload_size, &trailing) != 3 ||
        std::strcmp(protocol, "CRBC1") != 0)
    {
        reply(client, "ERR expected CRBC1 header\n");
        return;
    }

    if (!valid_upload_token() ||
        !constant_time_equal(supplied_token, CONFIG_CARBLANG_UPLOAD_TOKEN))
    {
        reply(client, "ERR unauthorized\n");
        return;
    }
    if (payload_size < 6 || payload_size > max_upload_size)
    {
        reply(client, "ERR bytecode size outside allowed range\n");
        return;
    }

    std::string payload(static_cast<size_t>(payload_size), '\0');
    if (!receive_exact(client, payload.data(), payload.size()))
    {
        reply(client, "ERR incomplete upload\n");
        return;
    }

    try
    {
        std::istringstream bytecode(std::move(payload), std::ios::in | std::ios::binary);
        Chunk chunk = read_bytecode(bytecode);
        static VM vm;
        static bool vm_initialized = false;
        if (!vm_initialized)
        {
            vm.init_globals();
            vm_initialized = true;
        }
        vm.interpret(chunk);
        reply(client, "OK script completed\n");
    }
    catch (const std::exception &exception)
    {
        ESP_LOGE(tag, "Bytecode execution failed: %s", exception.what());
        reply(client, "ERR bytecode rejected or execution failed\n");
    }
    shell_redraw_prompt();
}

void server_task(void *)
{
    while (true)
    {
        xEventGroupWaitBits(wifi_events, connected_bit, pdFALSE, pdTRUE, portMAX_DELAY);
        const int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (listener < 0)
        {
            ESP_LOGE(tag, "socket() failed: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int reuse = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(server_port);
        if (bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
            listen(listener, 1) != 0)
        {
            ESP_LOGE(tag, "Unable to bind TCP port %u: errno %d", server_port, errno);
            close(listener);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        while (wifi_connected)
        {
            sockaddr_in peer{};
            socklen_t peer_size = sizeof(peer);
            const int client = accept(listener, reinterpret_cast<sockaddr *>(&peer), &peer_size);
            if (client < 0)
            {
                if (wifi_connected)
                    ESP_LOGW(tag, "accept() failed: errno %d", errno);
                break;
            }
            timeval receive_timeout{15, 0};
            setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
                       &receive_timeout, sizeof(receive_timeout));
            handle_client(client);
            shutdown(client, SHUT_RDWR);
            close(client);
        }
        shutdown(listener, SHUT_RDWR);
        close(listener);
    }
}

bool start_wifi_stack()
{
    if (wifi_stack_ready)
        return true;

    // To keep things clean, annoying text keeps popping
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_WARN);

    if (!init_nvs())
        return false;
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;
    if (!esp_netif_create_default_wifi_sta())
        return false;
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK)
        return false;

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, nullptr));
    wifi_stack_ready = true;
    return true;
}

bool apply_wifi_credentials()
{
    if (wifi_started)
        return false;
    wifi_config_t station_config{};
    std::strncpy(reinterpret_cast<char *>(station_config.sta.ssid),
                 wifi_ssid, sizeof(station_config.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char *>(station_config.sta.password),
                 wifi_password, sizeof(station_config.sta.password) - 1);
    station_config.sta.threshold.authmode = wifi_password[0]
        ? WIFI_AUTH_WPA2_PSK
        : WIFI_AUTH_OPEN;
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
        return false;
    err = esp_wifi_set_config(WIFI_IF_STA, &station_config);
    if (err != ESP_OK)
        return false;
    wifi_auto_reconnect = true;
    wifi_connecting = true;
    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        wifi_auto_reconnect = false;
        wifi_connecting = false;
        wifi_reconfiguring = false;
        return false;
    }
    wifi_started = true;
    return true;
}

bool start_station_for_scan()
{
    if (!start_wifi_stack())
        return false;
    if (wifi_started)
        return true;
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK)
        return false;
    wifi_auto_reconnect = false;
    const esp_err_t err = esp_wifi_start();
    if (err != ESP_OK)
        return false;
    wifi_started = true;
    return true;
}

void reset_connection_status()
{
    wifi_connected = false;
    wifi_connecting = false;
    xEventGroupClearBits(wifi_events, connected_bit);
    std::strncpy(address_text, unavailable, sizeof(address_text) - 1);
    address_text[sizeof(address_text) - 1] = '\0';
}
}

extern "C" void start_script_server(void)
{
    wifi_events = xEventGroupCreate();
    if (!wifi_events)
    {
        ESP_LOGE(tag, "Unable to create WiFi event group");
        return;
    }

    // Keep the size like this, less will cause Carblang to not execute right
    if (xTaskCreate(server_task, "crb_server", 32768, nullptr, 5, nullptr) == pdPASS)
        server_task_started = true;
    else
        ESP_LOGE(tag, "Unable to create bytecode server task");

    if (!valid_upload_token())
        ESP_LOGW(tag, "Set a private upload token of 16 to 95 characters in menuconfig");
    else
        ESP_LOGI(tag, "Set WiFi with \"wifi set ssid\" and \"wifi set password\", then run \"wifi connect\"");
}

extern "C" bool carblang_wifi_set_ssid(const char *ssid)
{
    if (!ssid || !ssid[0] || std::strlen(ssid) > 32)
        return false;
    std::strcpy(wifi_ssid, ssid);
    return true;
}

extern "C" bool carblang_wifi_set_password(const char *password)
{
    if (!password || std::strlen(password) > 64)
        return false;
    std::strcpy(wifi_password, password);
    return true;
}

extern "C" bool carblang_wifi_connect(void)
{
    if (!wifi_ssid[0])
        return false;
    if (!start_wifi_stack())
        return false;
    if (wifi_started)
    {
        wifi_auto_reconnect = false;
        wifi_reconfiguring = true;
        reset_connection_status();
        const esp_err_t stop_err = esp_wifi_stop();
        if (stop_err != ESP_OK && stop_err != ESP_ERR_WIFI_NOT_STARTED)
        {
            wifi_reconfiguring = false;
            ESP_LOGE(tag, "Could not stop WiFi before applying credentials: %s",
                     esp_err_to_name(stop_err));
            return false;
        }
        wifi_started = false;
    }
    if (!apply_wifi_credentials())
    {
        wifi_auto_reconnect = false;
        wifi_reconfiguring = false;
        return false;
    }
    return true;
}

extern "C" bool carblang_wifi_disconnect(void)
{
    wifi_auto_reconnect = false;
    wifi_connecting = false;
    if (!wifi_started)
    {
        reset_connection_status();
        return true;
    }
    const esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_STARTED)
    {
        ESP_LOGE(tag, "Could not disconnect WiFi: %s", esp_err_to_name(err));
        return false;
    }
    wifi_started = false;
    reset_connection_status();
    return true;
}

extern "C" void carblang_wifi_status(void)
{
    if (wifi_connected)
    {
        wifi_ap_record_t ap{};
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
            std::printf("WiFi: connected\nConnected SSID: %.*s\n",
                        static_cast<int>(sizeof(ap.ssid)),
                        reinterpret_cast<const char *>(ap.ssid));
        else
            std::printf("WiFi: connected\n");
        if (wifi_ssid[0])
            std::printf("Configured SSID: %s\n", wifi_ssid);
        std::printf("IP address: %s\n", address_text);
        if (!server_task_started)
            std::printf("Upload server: unavailable (task did not start)\n");
        else if (!valid_upload_token())
            std::printf("Upload server: token not configured\n");
        else
            std::printf("Upload server: %s:3333\n", address_text);
        return;
    }

    if (wifi_connecting)
    {
        std::printf("WiFi: connecting/retrying\nSSID: %s\n", wifi_ssid);
        return;
    }
    if (wifi_started)
    {
        std::printf("WiFi: disconnected (station is running)\n");
        if (wifi_ssid[0])
            std::printf("Configured SSID: %s\n", wifi_ssid);
        return;
    }
    std::printf("WiFi: off\n");
    if (wifi_ssid[0])
        std::printf("Configured SSID: %s\n", wifi_ssid);
    else
        std::printf("No SSID configured, use \"wifi set ssid <ssid>\"\n");
}

extern "C" bool carblang_wifi_scan(void)
{
    if (!start_station_for_scan())
    {
        std::printf("Could not start WiFi scan.\n");
        return false;
    }

    wifi_scan_config_t config{};
    config.show_hidden = true;
    const esp_err_t scan_err = esp_wifi_scan_start(&config, true);
    if (scan_err != ESP_OK)
    {
        std::printf("WiFi scan failed: %s\n", esp_err_to_name(scan_err));
        return false;
    }

    uint16_t ap_count = 0;
    esp_err_t err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK)
    {
        std::printf("Could not get scan results: %s\n", esp_err_to_name(err));
        return false;
    }
    if (ap_count == 0)
    {
        std::printf("No WiFi networks found.\n");
        return true;
    }

    auto *records = static_cast<wifi_ap_record_t *>(
        std::calloc(ap_count, sizeof(wifi_ap_record_t)));
    if (!records)
    {
        std::printf("Not enough memory to list WiFi networks.\n");
        return false;
    }
    uint16_t returned_count = ap_count;
    err = esp_wifi_scan_get_ap_records(&returned_count, records);
    if (err != ESP_OK)
    {
        std::free(records);
        std::printf("Could not read scan results: %s\n", esp_err_to_name(err));
        return false;
    }

    std::printf("Nearby WiFi networks: %u\n", static_cast<unsigned>(returned_count));
    for (uint16_t i = 0; i < returned_count; ++i)
    {
        const char *ssid = reinterpret_cast<const char *>(records[i].ssid);
        if (records[i].ssid[0] == '\0')
            ssid = "(hidden)";
        std::printf("%u. %-32.32s  RSSI %d dBm  channel %u  auth %u\n",
                    static_cast<unsigned>(i + 1), ssid,
                    static_cast<int>(records[i].rssi),
                    static_cast<unsigned>(records[i].primary),
                    static_cast<unsigned>(records[i].authmode));
    }
    std::free(records);
    return true;
}

extern "C" bool script_server_is_connected(void)
{
    return wifi_connected;
}

extern "C" const char *script_server_ip(void)
{
    return address_text;
}
