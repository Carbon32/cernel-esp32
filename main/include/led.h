#pragma once

#include <stdint.h>

#define NATIVE_LED GPIO_NUM_2

void init_native_led();
void flash_native_led();
void switch_native_led();
void set_native_led(int enabled);

void carblang_led_on(void);
void carblang_led_off(void);
void carblang_led_toggle(void);