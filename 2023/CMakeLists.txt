cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

set(YEAR 2023)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pico)
  ween_executable(cheshire cheshire.cpp)
  ween_executable(pocket-watch pocket-watch.cpp)
  ween_executable(tweetle tweetle.cpp)
else()
endif()

ween_executable(animation animation.cpp)
