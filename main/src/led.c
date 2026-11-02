#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static int8_t led_state = 0;

void init_native_led() {
    gpio_reset_pin(NATIVE_LED);
    gpio_set_direction(NATIVE_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(NATIVE_LED, led_state);
}

void flash_native_led() {
    gpio_set_level(NATIVE_LED, 1); 
    vTaskDelay(pdMS_TO_TICKS(500));

    gpio_set_level(NATIVE_LED, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

void switch_native_led()
{
    led_state = !led_state;
    gpio_set_level(NATIVE_LED, led_state);
}