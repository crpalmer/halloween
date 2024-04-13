
cmake_minimum_required(VERSION 3.12)

set(YEAR 2019)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pi)
  ween_executable(fogger fogger.c)
endif()
