cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

set(YEAR 2025)

if("${PLATFORM}" STREQUAL pico)
  ween_executable(mushrooms mushrooms.cpp)
else()
  #ween_executable(animation animation.cpp animation-setup.cpp)
  #ween_executable_libraries(animation animation-station)
  #add_compile_definitions("ANIMATION_STATION_CSS_FILENAME=\"${CMAKE_CURRENT_LIST_DIR}/ui.css\"")
endif()
