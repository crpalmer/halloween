
cmake_minimum_required(VERSION 3.12)

set(YEAR 2021)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pi)
  ween_executable(alien alien.cpp)
  ween_executable(animation animation.cpp)
  ween_executable(band band.cpp)
  ween_executable(coffin coffin.c)
  ween_executable(jacob jacob.cpp)
  ween_executable(talker-and-fogger talker-and-fogger.cpp)
endif()
