cmake_minimum_required(VERSION 3.12)

if("${PLATFORM}" STREQUAL "pico")
  add_executable(claw-pico
    claw-pico.cpp
  )
  pico_enable_stdio_usb(claw-pico 1)
  pico_enable_stdio_uart(claw-pico 0)

  platform_executable(claw-pico)
  target_link_libraries(claw-pico
    lib-pi
    lib-pi-threads
  )
else()
  add_executable(claw
    claw.cpp
  )
  platform_executable(claw)
  target_link_libraries(claw
    lib-halloween
    lib-pi
    lib-pi-audio
    lib-pi-net
    lib-pi-threads
  )
endif()
