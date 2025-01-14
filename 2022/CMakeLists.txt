
cmake_minimum_required(VERSION 3.12)

set(YEAR 2022)

if("${PLATFORM}" STREQUAL pico)
  ween_executable(doug doug.cpp)
  ween_executable(sign-light sign-light.cpp)
else()
#  ween_executable(animation animation.cpp)
endif()
