cmake_minimum_required (VERSION 2.8)

project (DarmaDebugBackends)
# The version number.
set (DarmaDebugBackends_VERSION_MAJOR 0)
set (DarmaDebugBackends_VERSION_MINOR 2)

set (CMAKE_MACOSX_RPATH On)

# Compiler-specific C++11 activation.
set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} -std=c++1y")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if (NOT (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8))
    message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.8 or greater.")
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else ()
  message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif ()


if (DARMA_ALL_IN_ONE)
set (DARMA_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/src)
set (BACKENDS_ROOT ${CMAKE_SOURCE_DIR}/src/reference_backends)
include_directories( ${BACKENDS_ROOT} ${CMAKE_BINARY_DIR}/src/reference_backends )
else() #built as standalone
set (BACKENDS_ROOT ${CMAKE_SOURCE_DIR})
include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} )
if (NOT DARMA_FRONTEND_DIR)
message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND_DIR=")
endif()
endif()

include_directories( ${DARMA_FRONTEND_DIR}/include )
include_directories( ${DARMA_FRONTEND_DIR}/include/tinympl )
include_directories( ${DARMA_FRONTEND_DIR}/include/darma )

option(SERIAL_BACKEND "Whether to compile/link debug serial backend" Off)

if (SERIAL_BACKEND)
add_subdirectory(serial)
endif()

if (NOT SERIAL_BACKEND)
message(FATAL_ERROR "Must build with -DSERIAL_BACKEND=On")
endif()

