
cmake_minimum_required(VERSION 3.12)

set(YEAR 2022)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pico)
  ween_executable(doug doug.cpp)
  ween_executable(sign-light sign-light.cpp)
  ween_executable_libraries(sign-light pico_multicore)
else()
#  ween_executable(animation animation.cpp)
endif()
