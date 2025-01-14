cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

set(YEAR 2024)

if("${PLATFORM}" STREQUAL pico)
  ween_executable(fart-blaster fart-blaster.cpp)
  ween_executable_libraries(fart-blaster hardware_uart)

  ween_executable(freeze-ray freeze-ray.cpp)
  ween_executable(ice-blast ice-blast.cpp)
  ween_executable(vector vector.cpp)
else()
  ween_executable(fart-blaster-trigger fart-blaster-trigger.cpp)

  ween_executable(animation animation.cpp animation-setup.cpp)
  ween_executable_libraries(animation animation-station)
  add_compile_definitions("ANIMATION_STATION_CSS_FILENAME=\"${CMAKE_CURRENT_LIST_DIR}/ui.css\"")

  ween_executable(gus gus.cpp)
  ween_executable_libraries(gus animation-station)
endif()
