
cmake_minimum_required(VERSION 3.12)

set(YEAR 2020)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pi)
  ween_executable(animation animation.cpp)
  ween_executable(fogger fogger.c)
  ween_executable(hynopops hypnopops.c)
  ween_executable(witch witch.cpp)
endif()
