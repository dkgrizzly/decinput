cmake_minimum_required(VERSION 3.16)

include(cmake/pico_sdk_import.cmake)

project(vaxtops2)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPICO_USE_MALLOC_MUTEX=1")

add_executable(vaxtops2)
target_sources(vaxtops2 PUBLIC
        main.c mouse.c tablet.c keyboard.c
        )

# pull in common dependencies
target_link_libraries(vaxtops2
        pico_stdlib
        pico_multicore
        hardware_timer
        hardware_uart
        hardware_pwm
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(vaxtops2)

pico_enable_stdio_usb(vaxtops2 1)
