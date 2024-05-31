cmake_minimum_required(VERSION 3.12)

project(claw)

add_compile_definitions(Wall Werror)

if("${PLATFORM}" STREQUAL "pi")
  add_executable(claw
    claw.cpp
  )
  platform_executable(claw)
  target_link_libraries(claw
    halloween
  )
else()
  add_executable(claw-pico
    claw-pico.cpp
  )
  pico_enable_stdio_usb(claw-pico 1)
  pico_enable_stdio_uart(claw-pico 0)

  platform_executable(claw-pico)
  target_link_libraries(claw-pico lib-pi)
endif()
