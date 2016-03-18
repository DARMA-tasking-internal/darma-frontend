cmake_minimum_required (VERSION 2.8)

project (DarmaBackendValidation)
# The version number.
set (DarmaBackendValidation_VERSION_MAJOR 0)
set (DarmaBackendValidation_VERSION_MINOR 2)

enable_testing()

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
include_directories( ${VALIDATION_SRC_ROOT} )
include_directories( ${VALIDATION_BIN_ROOT} )
include_directories( ${GTEST_DIR}/include )

function(add_unit_test test_name)
add_executable(${test_name} ${test_name}.cc main.cc)
target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "--gtest_repeat=10")
set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
  FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is"
  PASS_REGULAR_EXPRESSION "PASSED")
endfunction()
#
add_unit_test(test_darma_backend_initialize)
add_unit_test(test_register_task)
add_unit_test(test_runtime_release_handle)
add_unit_test(test_get_running_task)
add_unit_test(test_fetchers)

