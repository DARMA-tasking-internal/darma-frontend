cmake_minimum_required (VERSION 2.8)

project (DarmaBackendValidation)
# The version number.
set (DarmaBackendValidation_VERSION_MAJOR 0)
set (DarmaBackendValidation_VERSION_MINOR 2)


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
set (VALIDATION_SRC_ROOT ${CMAKE_SOURCE_DIR}/examples)
set (VALIDATION_BIN_ROOT ${CMAKE_BINARY_DIR}/examples)
set (DARMA_FRONTEND ${CMAKE_SOURCE_DIR}/src)

#figure out which backend
include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends )
if (STL_THREADS_BACKEND)
include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends/stl_threads )
set (DARMA_BACKEND_LIBNAME darma_stl_threads_backend)
elseif (MOCK_BACKEND)
set (DARMA_BACKEND_LIBNAME darma_mock_backend)
include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends/mock )
set (EXTRA_LIBS )
elseif (SERIAL_BACKEND)
set (DARMA_BACKEND_LIBNAME darma_serial_backend)
include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends/serial )
endif()

else() #built as standalone
set (VALIDATION_SRC_ROOT ${CMAKE_SOURCE_DIR})
set (VALIDATION_BIN_ROOT ${CMAKE_BINARY_DIR})

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
include_directories( ${VALIDATION_SRC_ROOT} )
include_directories( ${VALIDATION_BIN_ROOT} )

function(add_unit_test test_name)
add_executable(${test_name} ${test_name}.cc)
target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "")
set_tests_properties(${test_name} PROPERTIES TIMEOUT 30 FAIL_REGULAR_EXPRESSION "FAILURE" PASS_REGULAR_EXPRESSION "PASSED")
endfunction()

#add_unit_test(test_darma_backend_initialize)
#add_unit_test(test_register_task)
#add_unit_test(test_runtime_release_handle)
