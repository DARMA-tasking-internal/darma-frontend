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
set (DARMA_FRONTEND ${CMAKE_SOURCE_DIR}/frontend)
set (BACKENDS_ROOT ${CMAKE_SOURCE_DIR}/backends)
include_directories( ${CMAKE_SOURCE_DIR}/backends ${CMAKE_BINARY_DIR}/backends )
else() #built as standalone
set (BACKENDS_ROOT ${CMAKE_SOURCE_DIR})
include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} )
if (NOT DARMA_FRONTEND)
message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND=")
endif()
endif()

include_directories( ${DARMA_FRONTEND}/include )
include_directories( ${DARMA_FRONTEND}/include/tinympl )
include_directories( ${DARMA_FRONTEND}/include/darma )

option(STL_THREADS_BACKEND "Whether to compile/link debug backend based on SLT threads" Off)
option(MOCK_BACKEND "Whether to compile/link mock backend for compiling only" Off)

if (STL_THREADS_BACKEND)
add_subdirectory(stl_threads)
endif()

if (MOCK_BACKEND)
add_subdirectory(mock)
endif()

if ((NOT STL_THREADS_BACKEND) AND (NOT MOCK_BACKEND))
message(FATAL_ERROR "Must at least build either define =On for MOCK_BACKEND or STL_THREADS_BACKEND")
endif()

install (FILES common/stream_key.h DESTINATION include/common)

