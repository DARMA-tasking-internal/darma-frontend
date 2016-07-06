cmake_minimum_required (VERSION 2.8)

project (DarmaDebugBackends)
# The version number.
set (DarmaDebugBackends_VERSION_MAJOR 0)
set (DarmaDebugBackends_VERSION_MINOR 2)

set (CMAKE_MACOSX_RPATH On)

include (../../SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

if (DARMA_ALL_IN_ONE)
set (DARMA_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/src)
set (BACKENDS_ROOT ${CMAKE_SOURCE_DIR}/src/reference_backends)
include_directories( ${BACKENDS_ROOT} ${CMAKE_BINARY_DIR}/src/reference_backends )
include_directories( ${CMAKE_SOURCE_DIR}/backend_validation/using_mock_frontend )
else() #built as standalone
set (BACKENDS_ROOT ${CMAKE_SOURCE_DIR})
include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} )
if (NOT DARMA_FRONTEND_DIR)
message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND_DIR=")
endif()
include_directories( ${DARMA_FRONTEND_DIR}/include/darma/backend_validation )
include_directories( ${DARMA_FRONTEND_DIR}/include/darma/backend_validation/using_mock_frontend )
endif()

include_directories( ${DARMA_FRONTEND_DIR}/include )
include_directories( ${DARMA_FRONTEND_DIR}/include/tinympl )
include_directories( ${DARMA_FRONTEND_DIR}/include/darma )

option(SERIAL_BACKEND "Whether to compile/link debug serial backend" Off)
option(THREADS_BACKEND "Whether to compile/link debug threads backend" Off)

if (SERIAL_BACKEND)
add_subdirectory(serial)
endif()

if (THREADS_BACKEND)
add_subdirectory(threads)
endif()

if (NOT SERIAL_BACKEND AND NOT THREADS_BACKEND)
message(FATAL_ERROR "Must build with -DSERIAL_BACKEND=On or -DTHREADS_BACKEND=On")
endif()

