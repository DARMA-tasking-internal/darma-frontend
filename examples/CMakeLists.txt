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

set (DARMA_BACKEND ${CMAKE_SOURCE_DIR}/backends)
set (DARMA_FRONTEND ${CMAKE_SOURCE_DIR}/frontend)

#figure out which backend
include_directories( ${CMAKE_SOURCE_DIR}/backends )
if (STL_THREADS_BACKEND)
include_directories( ${CMAKE_SOURCE_DIR}/backends/stl_threads )
set (IN_TREE_BACKEND darma_stl_threads_backend)
elseif (MOCK_BACKEND)
set (IN_TREE_BACKEND darma_mock_backend)
include_directories( ${CMAKE_SOURCE_DIR}/backends/mock )
set (EXTRA_LIBS )
endif()

else() #built as standalone
set (EXAMPLES_SRC_ROOT ${CMAKE_SOURCE_DIR})
set (EXAMPLES_BIN_ROOT ${CMAKE_BINARY_DIR})

if (NOT DARMA_FRONTEND)
message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND=")
endif()

if (NOT DARMA_BACKEND)
message(FATAL_ERROR "Must define prefix install dir for backend via -DDARMA_BACKEND=")
endif()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${DARMA_BACKEND}/lib")

if (NOT DARMA_BACKEND_LIBNAME)
message(FATAL_ERROR "Must give library name for backend, e.g. for libdarma_backend_test.a give -DDARMA_BACKEND_LIBNAME=darma_backend_test")
endif()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -l${DARMA_BACKEND_LIBNAME}")
endif() #DARMA_ALL_IN_ONE

include_directories( ${DARMA_FRONTEND}/include )
include_directories( ${DARMA_FRONTEND}/include/tinympl )
include_directories( ${DARMA_FRONTEND}/include/darma )
include_directories( ${DARMA_BACKEND}/include )
include_directories( ${EXAMPLES_SRC_ROOT} )
include_directories( ${EXAMPLES_BIN_ROOT} )



add_subdirectory(stencil_1d)
#add_subdirectory(simple_pic)


