add_executable(theme-player theme-player.cpp)
platform_executable(theme-player)
target_link_libraries(theme-player
  lib-pi
  lib-pi-audio
  lib-pi-threads
)
