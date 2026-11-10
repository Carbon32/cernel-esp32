#include "led.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

void switch_native_led() { set_native_led(!led_state); }

void set_native_led(int enabled) {
  led_state = enabled != 0;
  gpio_set_level(NATIVE_LED, led_state);
}

void carblang_led_on(void) { set_native_led(1); }
void carblang_led_off(void) { set_native_led(0); }
void carblang_led_toggle(void) { switch_native_led(); }