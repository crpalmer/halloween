
cmake_minimum_required(VERSION 3.12)

set(YEAR 2020)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pi)
#  ween_executable(animation animation.cpp)
  ween_executable(hynopops hypnopops.cpp)
  ween_executable(witch witch.cpp)
endif()

if(HAVE_NETWORKING)
  ween_executable(fogger fogger.cpp)
endif()
