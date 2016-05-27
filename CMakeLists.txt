cmake_minimum_required (VERSION 2.8)

project (Darma)
# The version number.
set (Darma_VERSION_MAJOR 0)
set (Darma_VERSION_MINOR 2)

include(CTest)

enable_testing()

include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_BINARY_DIR}/src/include )

# indicate that CMAKE was used, so config-generated.h can be used
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDARMA_CONFIG_GENERATED_H_AVAILABLE" )

set(DARMA_ALL_IN_ONE On)

option (CTEST_SKIP_EXAMPLES "Skip the examples when running ctest." OFF)

add_subdirectory(src)

if (GTEST_DIR)
add_subdirectory(backend_validation)
else()
message("-- GTEST_DIR not given for GoogleTest installation - skipping test build")
endif()

add_subdirectory(examples)

