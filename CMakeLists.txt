cmake_minimum_required(VERSION 3.12)

include(lib/platform.cmake)

project(halloween)

add_subdirectory(lib)
add_subdirectory(claw)
add_subdirectory(2023)
add_subdirectory(2022)
add_subdirectory(2021)
add_subdirectory(2020)
add_subdirectory(2019)

add_compile_definitions(Wall Werror)

if("${PLATFORM}" STREQUAL "pi")
  set(LIB_OBJS
    animation-station.cpp
    animation-main.c
    fogger.c
    talker.cpp
    tentacle.cpp
  )
endif()

add_library(halloween STATIC
  ${LIB_OBJS}
  ween-time.c
)

target_include_directories(halloween PUBLIC .)

target_link_libraries(
    halloween
    ${PLATFORM_LIBS}
    pi
)

