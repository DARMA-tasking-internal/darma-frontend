cmake_minimum_required (VERSION 2.8)

project (DarmaExamples)
# The version number.
set (DarmaExamples_VERSION_MAJOR 0)
set (DarmaExamples_VERSION_MINOR 2)


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
set (EXAMPLES_SRC_ROOT ${CMAKE_SOURCE_DIR}/examples)
set (EXAMPLES_BIN_ROOT ${CMAKE_BINARY_DIR}/examples)
set (DARMA_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/src)

#figure out which backend
include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends )
if (STL_THREADS_BACKEND) 
set (backend_name stl_threads)
elseif (MOCK_BACKEND)    
set (backend_name mock)
elseif (SERIAL_BACKEND)  
set (backend_name serial)
endif()
set(backend_dir ${CMAKE_SOURCE_DIR}/src/reference_backends/${backend_name})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${backend_dir}")
set(DARMA_BACKEND_LIBNAME darma_${backend_name}_backend)
include_directories(${backend_dir})

else() #built as standalone
set (EXAMPLES_SRC_ROOT ${CMAKE_SOURCE_DIR})
set (EXAMPLES_BIN_ROOT ${CMAKE_BINARY_DIR})

if (NOT DARMA_FRONTEND_DIR)
message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND=")
endif()

if (NOT DARMA_BACKEND_DIR)
message(FATAL_ERROR "Must define prefix install dir for backend via -DDARMA_BACKEND=")
endif()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${DARMA_BACKEND_DIR}/lib")

if (NOT DARMA_BACKEND_LIBNAME)
message(FATAL_ERROR "Must give library name for backend, e.g. for libdarma_backend_test.a give -DDARMA_BACKEND_LIBNAME=darma_backend_test")
endif()
endif() #DARMA_ALL_IN_ONE

include_directories( ${DARMA_FRONTEND_DIR}/include )
include_directories( ${DARMA_FRONTEND_DIR}/include/tinympl )
include_directories( ${DARMA_FRONTEND_DIR}/include/darma )
include_directories( ${EXAMPLES_SRC_ROOT} )
include_directories( ${EXAMPLES_BIN_ROOT} )


add_subdirectory(stencil_1d)
add_subdirectory(simple_hello)
add_subdirectory(darma_hello)
add_subdirectory(darma_rank)
add_subdirectory(poisson_1d_thomasAlgorithm)
# add_subdirectory(heat_1d)
add_subdirectory(simple_pic)

