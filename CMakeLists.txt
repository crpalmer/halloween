cmake_minimum_required(VERSION 3.12)

include(lib/platform.cmake)

add_subdirectory(lib)

project(halloween)

add_subdirectory(claw)
add_subdirectory(2023)
add_subdirectory(2022)
add_subdirectory(2021)
add_subdirectory(2020)
add_subdirectory(2019)

if("${PLATFORM}" STREQUAL "pi")
  set(LIB_OBJS
    fogger.c
    talker.cpp
  tentacle.cpp
  )
endif()

add_library(halloween STATIC
  animation-station.cpp
  ween-time.c
  ${LIB_OBJS}
)

target_include_directories(halloween PUBLIC .)

target_link_libraries(halloween
  pi-core
  pi-net
  pi-threads
)

if("${PLATFORM}" STREQUAL "pi")
  target_link_libraries(halloween pi-net)
endif()
