cmake_minimum_required (VERSION 2.8)

# Silence an error about the version option to project
# this will need to be changed if we ever bump the version to 3.0 and put the
# version stuff in the project command itself
if (NOT CMAKE_VERSION VERSION_LESS 3.0)
  cmake_policy(SET CMP0048 OLD)
endif()

project (DarmaFrontend)
# The version number.
set (DarmaFrontend_VERSION_MAJOR 0)
set (DarmaFrontend_VERSION_MINOR 4)

set (DARMA_FE_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${DARMA_FE_REPO_ROOT}/cmake-modules)


option (DARMA_FE_ENABLE_TESTS "Enable build of frontend tests" Off)

if (DARMA_FE_ENABLE_TESTS)
  message("DARMA frontend tests will be built.")
  set(GTEST_ROOT ${GTEST_DIR})
  set(GMOCK_ROOT ${GTEST_DIR})
  find_package(GTest REQUIRED)
  find_package(GMock REQUIRED)
  include(CTest)
  enable_testing()
else()
  message("DARMA frontend tests will be skipped.")
endif()

# indicate that CMAKE was used, so config-generated.h can be used
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDARMA_CONFIG_GENERATED_H_AVAILABLE" )

option (DARMA_GENERATE_OO_MACRO_FILES 
    "Generate OO macro expansions for testing and debugging purposes in test_oo"
    OFF
)


option(DARMA_COVERAGE "Whether to setup coverage targets for tests" Off)

if (DARMA_COVERAGE)
include(CodeCoverage)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

include (CheckCXXCompilerFlag)
include (CheckCXXSourceCompiles)

if (NOT CMAKE_VERSION VERSION_LESS 3.4)
CHECK_CXX_COMPILER_FLAG("-fconcepts" DARMA_HAS_CXX_CONCEPTS_FLAG)
endif()

add_subdirectory(src)

