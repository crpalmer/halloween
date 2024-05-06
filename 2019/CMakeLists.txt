cmake_minimum_required(VERSION 3.12)

set(YEAR 2019)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pi)
  # ween_executable(band band.cpp)   - Needs to be updated but I don't have the VSA files to test
endif()

ween_executable(fogger fogger.cpp)
  ween_executable(earth-invaders earth-invaders.cpp)
  ween_executable(paperboy paperboy.cpp)
  ween_executable(scow scow.cpp)
