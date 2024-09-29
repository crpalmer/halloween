cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

set(YEAR 2023)
include(../ween.cmake)

project(halloween-${YEAR})

if("${PLATFORM}" STREQUAL pico)
  ween_executable(cheshire cheshire.cpp)
  ween_executable(pocket-watch pocket-watch.cpp)
  ween_executable(tweetle tweetle.cpp)
  add_compile_definitions("ANIMATION_STATION_CSS_FILENAME=\"ui.css\"")
else()
  add_compile_definitions("ANIMATION_STATION_CSS_FILENAME=\"${CMAKE_CURRENT_LIST_DIR}/ui.css\"")
endif()

if(HAVE_NETWORKING)
  ween_executable(animation animation.cpp ${PLATFORM_DIR}-setup.cpp)
  ween_executable_libraries(animation animation-station)
endif()
