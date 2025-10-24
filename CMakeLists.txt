cmake_minimum_required(VERSION 3.12)

include(lib/platform.cmake)

project(halloween ASM C CXX)

include(ween.cmake)

add_subdirectory(lib)
add_subdirectory(lib-halloween)

if (HAVE_NETWORKING)
  add_subdirectory(animation-station)
  add_subdirectory(theme-player)
endif()

add_subdirectory(claw)
add_subdirectory(props)
add_subdirectory(2025)
add_subdirectory(2024)
add_subdirectory(2023)
add_subdirectory(2022)
add_subdirectory(2021)
add_subdirectory(2020)
add_subdirectory(2019)
