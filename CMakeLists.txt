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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${DARMA_FE_REPO_ROOT}/cmake-modules/coveralls/cmake)


option (DARMA_FE_ENABLE_TESTS "Enable build of frontend tests" Off)
option (DARMA_CT_ENABLE_TESTS "Enable compilation time tests" Off)
option (DARMA_ENABLE_COVERALLS "Enable coveralls coverage suport" Off)
option (DARMA_COVERALLS_UPLOAD "Upload coveralls results" Off)
mark_as_advanced(DARMA_COVERALLS_UPLOAD)

set (DARMA_TESTS False)
if (DARMA_FE_ENABLE_TESTS OR DARMA_CT_ENABLE_TESTS)
set (DARMA_TESTS True)
endif()

if (DARMA_TESTS)
  if (DARMA_FE_ENABLE_TESTS)
  message("DARMA frontend tests will be built.")
  set(GTEST_ROOT ${GTEST_DIR})
  set(GMOCK_ROOT ${GTEST_DIR})
  find_package(GTest REQUIRED)
  find_package(GMock REQUIRED)
  endif()
  if (DARMA_CT_ENABLE_TESTS)
  message("DARMA compile time tests will be included.")
  endif()
  include(CTest)
  enable_testing()
else()
  message("DARMA frontend tests will be skipped.")
endif()



if (DARMA_ENABLE_COVERALLS)
  if (!DARMA_FE_ENABLE_TESTS)
    message(FATAL_ERROR "Can't run coveralls coverage without tests enabled")
  endif()
  include(Coveralls)
  coveralls_turn_on_coverage()
  file(GLOB_RECURSE all_source_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/include/darma/*.h)
  coveralls_setup(
    "${all_source_files}"
    ${DARMA_COVERALLS_UPLOAD}
    "${PROJECT_SOURCE_DIR}/cmake-modules/coveralls/cmake"
  )
endif()

include (CheckCXXCompilerFlag)
include (CheckCXXSourceCompiles)

if (NOT CMAKE_VERSION VERSION_LESS 3.4)
  CHECK_CXX_COMPILER_FLAG("-fconcepts" DARMA_HAS_CXX_CONCEPTS_FLAG)
endif()

add_subdirectory(src)

add_library(darma_frontend INTERFACE)
target_include_directories(darma_frontend INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

# indicate that CMAKE was used, so config-generated.h can be used
target_compile_options(darma_frontend INTERFACE -DDARMA_CONFIG_GENERATED_H_AVAILABLE)

install(TARGETS darma_frontend EXPORT darma_frontend)
install(EXPORT darma_frontend DESTINATION cmake)

configure_file(
  cmake/darma_frontendConfig.cmake.in
  "${PROJECT_BINARY_DIR}/darma_frontendConfig.cmake" @ONLY
)
install(
  FILES
  "${PROJECT_BINARY_DIR}/darma_frontendConfig.cmake" 
  DESTINATION cmake 
)
 




