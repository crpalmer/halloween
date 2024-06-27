cmake_minimum_required(VERSION 3.12)

if("${PLATFORM}" STREQUAL "pico")
  include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
endif()

project(halloween)

include(lib/platform.cmake)

add_subdirectory(lib)
add_subdirectory(animation-station)

add_subdirectory(claw)
add_subdirectory(2024)
add_subdirectory(2023)
add_subdirectory(2022)
add_subdirectory(2021)
add_subdirectory(2020)
add_subdirectory(2019)

if("${PLATFORM}" STREQUAL "pi")
  set(LIB_OBJS
#    talker.cpp
    tentacle.cpp
  )
endif()

add_library(halloween STATIC
  fogger.cpp
  ween-time.c
  ${LIB_OBJS}
)

target_include_directories(halloween PUBLIC .)

target_link_libraries(halloween PUBLIC lib-pi)
