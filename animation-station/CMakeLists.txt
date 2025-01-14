add_library(animation-station INTERFACE)

target_include_directories(animation-station INTERFACE
   ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(animation-station INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/animation-station.cpp
  ${CMAKE_CURRENT_LIST_DIR}/animation-ui.cpp
)

target_link_libraries(animation-station INTERFACE
  lib-pi
  lib-pi-audio
  lib-pi-net
  lib-pi-threads
)
