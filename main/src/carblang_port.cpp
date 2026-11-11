#include "core/core.hpp"

#ifdef ESP_PLATFORM
    #include <cstdint>
    #include <cstddef>
    #include "driver/uart.h"
    #include "freertos/FreeRTOS.h"

    extern "C" void carblang_uart_write(const char *data, size_t size)
    {
        if (data && size)
            uart_write_bytes(UART_NUM_0, data, size);
    }

    extern "C" int carblang_uart_read_char(void)
    {
        uint8_t byte;
        if (uart_read_bytes(UART_NUM_0, &byte, 1, portMAX_DELAY) != 1)
            return -1;
        return byte;
    }
#endif

extern "C" void carblang_repl(void)
{
    Carblang carblang(true);
    carblang.interactive();
}
