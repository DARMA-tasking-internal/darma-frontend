cmake_minimum_required (VERSION 2.8)

project (DarmaFrontend)
# The version number.
set (DarmaFrontend_VERSION_MAJOR 0)
set (DarmaFrontend_VERSION_MINOR 2)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include )

add_subdirectory(include)
add_subdirectory(tests)
add_subdirectory(reference_backends)


