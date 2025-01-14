if("${PLATFORM}" STREQUAL "pi")
  set(LIB_OBJS
#    talker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tentacle.cpp
  )
endif()

add_library(lib-halloween INTERFACE)

target_sources(lib-halloween INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/fogger.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ween-time.c
  ${LIB_OBJS}
)

target_include_directories(lib-halloween INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(lib-halloween INTERFACE
  lib-pi
  lib-pi-threads
)
