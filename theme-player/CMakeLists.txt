cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

project(theme-player)

add_executable(theme-player theme-player.cpp)
platform_executable(theme-player)
target_link_libraries(theme-player lib-pi)
