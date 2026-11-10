#include "core/core.hpp"

#ifdef ESP_PLATFORM
    #include <cstddef>
    #include "driver/uart.h"

    extern "C" void carblang_uart_write(const char *data, size_t size)
    {
        if (data && size)
            uart_write_bytes(UART_NUM_0, data, size);
    }
#endif

extern "C" void carblang_repl(void)
{
    Carblang carblang(true);
    carblang.interactive();
}
