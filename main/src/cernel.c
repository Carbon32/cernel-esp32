#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "shell.h"

void app_main(void) {
  init_native_led();

  if (!uart_is_driver_installed(UART_NUM_0)) {
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0));
  }

  uart_vfs_dev_use_driver(UART_NUM_0);
  setvbuf(stdin, NULL, _IONBF, 0);

  start_script_server();
  shell_loop();
}