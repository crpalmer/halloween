if("${YEAR}" STREQUAL "")
  message(FATAL_ERROR "Set a YEAR for the build")
endif()

# ween_executable(exe_name {source files})
function(ween_executable exe_name)
  add_executable(${exe_name}-${YEAR} ${ARGN})
  platform_executable(${exe_name}-${YEAR})
  set_target_properties(${exe_name}-${YEAR} PROPERTIES OUTPUT_NAME ${exe_name})
  target_compile_definitions(${exe_name}-${YEAR} PUBLIC "CYW43_HOST_NAME=\"${exe_name}\"")
  if("${PLATFORM}" STREQUAL "pico")
    pico_enable_stdio_usb(${exe_name}-${YEAR} 1)
    pico_enable_stdio_uart(${exe_name}-${YEAR} 1)
    # create map/bin/hex file etc.
    pico_add_extra_outputs(${exe_name}-${YEAR})
  endif()
  target_link_libraries(
    ${exe_name}-${YEAR}
    halloween
  )
endfunction()

function(ween_executable_libraries exe_name)
  target_link_libraries(
    ${exe_name}-${YEAR}
    ${ARGN}
  )
endfunction()
