cmake_minimum_required (VERSION 2.8)

project (DarmaFrontend)
# The version number.
set (DarmaFrontend_VERSION_MAJOR 0)
set (DarmaFrontend_VERSION_MINOR 2)

include_directories( "${CMAKE_SOURCE_DIR}" ${CMAKE_BINARY_DIR} )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(include)

