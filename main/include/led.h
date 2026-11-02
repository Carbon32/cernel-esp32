#pragma once

#include <stdint.h>

#define NATIVE_LED GPIO_NUM_2

void init_native_led();
void flash_native_led();
void switch_native_led();