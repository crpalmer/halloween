# ween_executable(exe_name {source files})
function(ween_executable exe_name)
  add_executable(${exe_name}-${YEAR} ${ARGN})
  platform_executable(${exe_name}-${YEAR})
  set_target_properties(${exe_name}-${YEAR} PROPERTIES OUTPUT_NAME ${exe_name})
  target_compile_definitions(${exe_name}-${YEAR} PUBLIC "CYW43_HOST_NAME=\"${exe_name}\"")
  target_include_directories(${exe_name}-${YEAR} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
  if("${PLATFORM}" STREQUAL "pico")
    pico_enable_stdio_usb(${exe_name}-${YEAR} 1)
    pico_enable_stdio_uart(${exe_name}-${YEAR} 1)
    # create map/bin/hex file etc.
    pico_add_extra_outputs(${exe_name}-${YEAR})
  endif()
  target_link_libraries(${exe_name}-${YEAR} PUBLIC
    lib-halloween
    lib-pi
    lib-pi-audio
    lib-pi-net
    lib-pi-threads
  )
endfunction()

function(ween_executable_libraries exe_name)
  target_link_libraries(${exe_name}-${YEAR} PUBLIC
    ${ARGN}
  )
endfunction()
