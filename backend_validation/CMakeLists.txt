cmake_minimum_required (VERSION 2.8)

# Silence an error about the version option to project
# this will need to be changed if we ever bump the version to 3.0 and put thee
# version stuff in the project command itself
cmake_policy(VERSION 2.8)

project (DarmaBackendValidation)
# The version number.
set (DarmaBackendValidation_VERSION_MAJOR 0)
set (DarmaBackendValidation_VERSION_MINOR 2)

enable_testing()

include (../SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

set (VALIDATION_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set (VALIDATION_BIN_ROOT ${CMAKE_CURRENT_BINARY_DIR})

if (DARMA_ALL_IN_ONE)
set (DARMA_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/src)

#figure out which backend
include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends )
if (STL_THREADS_BACKEND) 
set (backend_name stl_threads)
elseif (MOCK_BACKEND)    
set (backend_name mock)
elseif (SERIAL_BACKEND)  
set (backend_name serial)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDARMA_SERIAL_BACKEND=1")
elseif (THREADS_BACKEND)  
set (backend_name threads)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDARMA_THREADS_BACKEND=1")
endif()
set(backend_dir ${CMAKE_SOURCE_DIR}/src/reference_backends/${backend_name})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${backend_dir}")
set(DARMA_BACKEND_LIBNAME darma_${backend_name}_backend)
include_directories(${backend_dir})
else() #built as standalone

if (NOT DARMA_FRONTEND_DIR)
message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND_DIR=")
endif()

if (NOT GTEST_DIR)
message(FATAL_ERROR "Must define prefix install dir for GoogleTest via -DGTEST_DIR=")
endif()

if (NOT DARMA_BACKEND_DIR)
message(FATAL_ERROR "Must define prefix install dir for backend via -DDARMA_BACKEND_DIR=")
endif()

include_directories( ${DARMA_BACKEND_DIR}/include )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${DARMA_BACKEND_DIR}/lib")

if (NOT DARMA_BACKEND_LIBNAME)
message(FATAL_ERROR "Must give library name for backend, e.g. for libdarma_backend_test.a give -DDARMA_BACKEND_LIBNAME=darma_backend_test")
endif()
endif() #DARMA_ALL_IN_ONE

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")

include_directories( ${DARMA_FRONTEND_DIR}/include )
include_directories( ${DARMA_FRONTEND_DIR}/include/tinympl )
include_directories( ${DARMA_FRONTEND_DIR}/include/darma )
include_directories( ${GTEST_DIR}/include )

add_subdirectory(using_real_frontend)

