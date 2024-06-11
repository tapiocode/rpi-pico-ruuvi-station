cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico-ruuvi-station VERSION 1.0 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

include_directories(pico-ruuvi-station PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ruuvi.endpoints.c/src
    )

add_executable(pico-ruuvi-station
    ruuvi.endpoints.c/src/ruuvi_endpoint_5.c
    ruuvitag_reader/ruuvitag_reader.c
    pico-ssd1306/ssd1306.c
    sparkline/sparkline.c
    button/button.c
    led/led.c
    main.c
    )

target_link_libraries(pico-ruuvi-station
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    hardware_i2c
    )

target_include_directories(pico-ruuvi-station PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )

pico_add_extra_outputs(pico-ruuvi-station)
