add_library(props INTERFACE)

target_include_directories(props INTERFACE
   ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(props INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/riser-prop.cpp
)

target_link_libraries(props INTERFACE
  lib-pi
  lib-pi-audio
  lib-pi-net
  lib-pi-threads
)
