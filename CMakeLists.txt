#
# Graphlib CMake Build
#
project(graphlib)
cmake_minimum_required(VERSION 2.6)

set(GRAPHLIB_SOURCES graphlib.c)

# Libraries and special compile flags.
add_library(lnlgraph SHARED ${GRAPHLIB_SOURCES})
set_target_properties(lnlgraph PROPERTIES
  COMPILE_FLAGS "-g")
#
#add_library(lnlgraph-c ${GRAPHLIB_SOURCES})
#set_target_properties(lnlgraph-c PROPERTIES COMPILE_FLAGS "-DNOSET")

# Executables
add_executable(graphdemo graphdemo.c)
target_link_libraries(graphdemo lnlgraph)
set_target_properties(graphdemo PROPERTIES
  COMPILE_FLAGS "-g")

add_executable(grmerge grmerge.c)
target_link_libraries(grmerge lnlgraph)
set_target_properties(grmerge PROPERTIES
  COMPILE_FLAGS "-g")

#add_executable(grmerge-c grmerge.c)
#target_link_libraries(grmerge-c lnlgraph-c)

# Installation
install(TARGETS graphdemo grmerge           DESTINATION bin)
install(TARGETS lnlgraph                    DESTINATION lib EXPORT graphlib-libs)
install(FILES   graphlib.h                  DESTINATION include)
install(EXPORT  graphlib-libs               DESTINATION lib/cmake/graphlib)

configure_file(
  ${PROJECT_SOURCE_DIR}/graphlib-config.cmake.in
  ${PROJECT_BINARY_DIR}/graphlib-config.cmake @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/graphlib-config.cmake DESTINATION lib/cmake/graphlib)

# Includes for entire build
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
