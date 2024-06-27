cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

set(YEAR 2024)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pico)
  ween_executable(fart-blaster fart-blaster.cpp)
endif()

#ween_executable(animation animation.cpp)
#ween_executable_libraries(animation animation-station)
