
##################################################
# Preliminary setup
##################################################

if (NOT GTEST_DIR)
  message("It is recommended to define the prefix install dir for GoogleTest via -DGTEST_DIR=")
else()
  include_directories( ${GTEST_DIR}/include )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")
endif()

find_package ( Threads REQUIRED )
find_package( PythonInterp )
find_python_module( jinja2 REQUIRED )

if( PYTHON_VERSION_STRING VERSION_LESS 2.7 )
   message(FATAL_ERROR "Can't generate OO headers for test_oo without Python2.7 or later")
endif()

include_directories(${DARMA_FE_REPO_ROOT}/src/tests/frontend_validation)

##################################################
# Store test names into UNIT_TESTS_NAMES
##################################################

function(add_compilation_time_tests test_name)
   set(UNIT_TESTS_NAMES ${UNIT_TESTS_NAMES} ${test_name} PARENT_SCOPE)
endfunction(add_compilation_time_tests)

##################################################
# Add unit tests for compilation
##################################################

add_compilation_time_tests(test_kwarg_functor)

##################################################
# Loop over tests
##################################################

foreach(test_name IN LISTS UNIT_TESTS_NAMES)

## Set source file name
   set(test_files "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cc")

## Create empty source file if required
   if (NOT EXISTS "${test_files}")
   file(WRITE "${test_files}" "")
   endif()

## Create target 
   add_library(${test_name}.measure EXCLUDE_FROM_ALL OBJECT ${test_files})
   
## Create custom command that will invoke the cmake test
   add_test("${test_name}.compile"
      COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/eval_comp_time.py" ${test_name} ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
      VERBATIM USES_TERMINAL
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/eval_comp_time.py" 
   )

endforeach()
