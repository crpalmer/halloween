cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

include(pico_sdk_import.cmake)

project(halloween20223)

pico_sdk_init()

add_definitions(-DPI_PICO)

function(util_common targetName)
    pico_enable_stdio_usb(${targetName} 1)
    pico_enable_stdio_uart(${targetName} 0)
    # create map/bin/hex file etc.
    pico_add_extra_outputs(${targetName})

    target_include_directories(${targetName} PRIVATE .)
    target_include_directories(${targetName} PRIVATE ../../lib)
    target_link_directories(${targetName} PRIVATE ../../lib/build)
    target_link_libraries(
	${targetName}
	PRIVATE pi
        pico_stdlib
	pico_multicore
        hardware_pwm
        hardware_pio
    )
endfunction()

function(util_c targetName)
    add_executable(${targetName} ${targetName}.c ${ARGV})
    util_common(${targetName})
endfunction()

function(util_cpp targetName)
    add_executable(${targetName} ${targetName}.cpp ${ARGV})
    util_common(${targetName})
endfunction()

util_cpp(cheshire)
util_cpp(pocket-watch)
util_cpp(tweetle)
