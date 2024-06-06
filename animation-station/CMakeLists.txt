cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

project(animation-station)

add_library(animation-station STATIC)

target_include_directories(animation-station PUBLIC
  .
)

target_sources(animation-station PRIVATE
  animation-station.cpp
)

target_link_libraries(animation-station PUBLIC
  lib-pi
)
