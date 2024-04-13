
cmake_minimum_required(VERSION 3.12)

set(YEAR 2019)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pi)
  ween_executable(animation animation.c)
  ween_executable(animation2 animation2.c)
  ween_executable(band band.c)
  ween_executable(factory factory.c)
  ween_executable(molar molar.c)
  ween_executable(orc orc.c)
endif()
