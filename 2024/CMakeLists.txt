cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

set(YEAR 2024)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pico)
  ween_executable(fart-blaster fart-blaster.cpp)
  ween_executable_libraries(fart-blaster hardware_uart)

  ween_executable(freeze-ray freeze-ray.cpp)
  ween_executable(ice-blast ice-blast.cpp)
  ween_executable(vector vector.cpp)
else()
  ween_executable(fart-blaster-trigger fart-blaster-trigger.cpp)
endif()

#ween_executable(animation animation.cpp)
#ween_executable_libraries(animation animation-station)
