cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

project(theme-player)

add_executable(theme-player theme-player.cpp)

pico_enable_stdio_usb(theme-player 1)
pico_enable_stdio_uart(theme-player 0)

platform_executable(theme-player)
target_link_libraries(theme-player lib-pi)

