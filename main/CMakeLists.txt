set(CARBLANG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../small-carblang")

idf_component_register(
    SRCS
        "src/cernel.c"
        "src/led.c"
        "src/hashmap.c"
        "src/shell.c"
        "src/cmd.c"
        "src/carblang_port.cpp"
        "src/script_server.cpp"
        "${CARBLANG_ROOT}/src/core/core.cpp"
        "${CARBLANG_ROOT}/src/core/error.cpp"
        "${CARBLANG_ROOT}/src/core/scanner.cpp"
        "${CARBLANG_ROOT}/src/core/token.cpp"
        "${CARBLANG_ROOT}/src/core/vm.cpp"
        "${CARBLANG_ROOT}/src/core/parser.cpp"
        "${CARBLANG_ROOT}/src/core/compiler.cpp"
        "${CARBLANG_ROOT}/src/core/module.cpp"
        "${CARBLANG_ROOT}/src/core/bytecode.cpp"
    INCLUDE_DIRS "include" "${CARBLANG_ROOT}/include"
    REQUIRES esp_driver_gpio esp_driver_uart esp_wifi esp_event esp_netif nvs_flash lwip
)
