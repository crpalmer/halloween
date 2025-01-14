
cmake_minimum_required(VERSION 3.12)

set(YEAR 2021)

if("${PLATFORM}" STREQUAL pi)
  ween_executable(alien alien.cpp)
#  ween_executable(animation animation.cpp)
  ween_executable(coffin coffin.cpp)
  ween_executable(jacob jacob.cpp)
  # ween_executable(talker-and-fogger talker-and-fogger.cpp) - talker needs to be updated but I don't know if we want it anymore...
endif()

if(HAVE_NETWORKING)
  ween_executable(band band.cpp)
endif()
