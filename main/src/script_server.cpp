#include "core/bytecode.hpp"

#include <cerrno>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

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
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

extern "C" void shell_redraw_prompt(void);

namespace
{
constexpr char tag[] = "carblang_net";
constexpr EventBits_t connected_bit = BIT0;
constexpr uint16_t server_port = 3333;
constexpr size_t max_upload_size = 24 * 1024;
constexpr size_t max_header_size = 192;
constexpr char unavailable[] = "not connected";
EventGroupHandle_t wifi_events;
std::atomic_bool wifi_connected{false};
char address_text[16] = "not connected";
char wifi_ssid[33]{};
char wifi_password[65]{};
bool wifi_stack_ready = false;
bool wifi_started = false;
bool wifi_reconfiguring = false;
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

bool save_wifi_value(const char *key, const char *value)
{
    if (!init_nvs())
        return false;
    nvs_handle_t handle;
    esp_err_t err = nvs_open("carblang", NVS_READWRITE, &handle);
    if (err == ESP_OK)
    {
        err = nvs_set_str(handle, key, value);
        if (err == ESP_OK)
            err = nvs_commit(handle);
        nvs_close(handle);
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(tag, "Unable to save WiFi setting: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

void load_wifi_settings()
{
    std::strncpy(wifi_ssid, CONFIG_CARBLANG_WIFI_SSID, sizeof(wifi_ssid) - 1);
    std::strncpy(wifi_password, CONFIG_CARBLANG_WIFI_PASSWORD, sizeof(wifi_password) - 1);
    nvs_handle_t handle;
    if (nvs_open("carblang", NVS_READONLY, &handle) != ESP_OK)
        return;
    size_t size = sizeof(wifi_ssid);
    if (nvs_get_str(handle, "wifi_ssid", wifi_ssid, &size) != ESP_OK)
        std::strncpy(wifi_ssid, CONFIG_CARBLANG_WIFI_SSID, sizeof(wifi_ssid) - 1);
    size = sizeof(wifi_password);
    if (nvs_get_str(handle, "wifi_password", wifi_password, &size) != ESP_OK)
        std::strncpy(wifi_password, CONFIG_CARBLANG_WIFI_PASSWORD, sizeof(wifi_password) - 1);
    nvs_close(handle);
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
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        std::strncpy(address_text, unavailable, sizeof(address_text) - 1);
        address_text[sizeof(address_text) - 1] = '\0';
        xEventGroupClearBits(wifi_events, connected_bit);
        if (wifi_started && !wifi_reconfiguring)
            esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        const auto *event = static_cast<const ip_event_got_ip_t *>(event_data);
        std::snprintf(address_text, sizeof(address_text), IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        xEventGroupSetBits(wifi_events, connected_bit);
        ESP_LOGI(tag, "Connected; bytecode server at %s:%u", address_text, server_port);
        shell_redraw_prompt();
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
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
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
    err = esp_wifi_start();
    if (err != ESP_OK)
        return false;
    wifi_started = true;
    return true;
}
}

extern "C" void start_script_server(void)
{
    if (!init_nvs())
        return;
    load_wifi_settings();
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

    if (wifi_ssid[0] != '\0')
    {
        if (!start_wifi_stack() || !apply_wifi_credentials())
            ESP_LOGE(tag, "Could not start WiFi, set credentials with wifi commands");
        if (!valid_upload_token())
            ESP_LOGW(tag, "WiFi is enabled, but uploads need a private token configured in menuconfig");
    }
    else if (!valid_upload_token())
        ESP_LOGW(tag, "Set a private upload token of 16 to 95 characters in menuconfig");
    else
        ESP_LOGI(tag, "Set WiFi with \"wifi set ssid\" and \"wifi set password\", then run \"wifi connect\"");
}

extern "C" bool carblang_wifi_set_ssid(const char *ssid)
{
    if (!ssid || !ssid[0] || std::strlen(ssid) > 32 || !save_wifi_value("wifi_ssid", ssid))
        return false;
    std::strcpy(wifi_ssid, ssid);
    return true;
}

extern "C" bool carblang_wifi_set_password(const char *password)
{
    if (!password || std::strlen(password) > 64 ||
        !save_wifi_value("wifi_password", password))
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
        wifi_reconfiguring = true;
        xEventGroupClearBits(wifi_events, connected_bit);
        wifi_connected = false;
        std::strncpy(address_text, unavailable, sizeof(address_text) - 1);
        address_text[sizeof(address_text) - 1] = '\0';
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
        wifi_reconfiguring = false;
        return false;
    }
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