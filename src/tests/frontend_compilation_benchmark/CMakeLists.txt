
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

add_compilation_time_tests(test_create_work)

##################################################
# Global target that depends on all unit tests
##################################################

add_custom_target(benchmarks ALL COMMENT "Generate all compilation benchmarks.")

##################################################
# Create empty include_var.h if needed
##################################################

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include_var.h")
   file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/include_var.h" "")
endif()

##################################################
# Loop over tests
##################################################

foreach(test_name IN LISTS UNIT_TESTS_NAMES)

## Create target 
   set(test_files "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cc")
   add_library(${test_name}.measure OBJECT ${test_files})
   
## Create custom command that will invoke the cmake test
   set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${test_name}.json")
   add_custom_command(OUTPUT ${output_file} 
      COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/eval_comp_time.py" ${test_name} ${CMAKE_COMMAND} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CACHEFILE_DIR}
      VERBATIM USES_TERMINAL
      DEPENDS "${test_files}" "${CMAKE_CURRENT_SOURCE_DIR}/include_var.h"
      COMMENT "Evaluating compilation time for ${test_name}"
   )

## Create a custom target and add it as a dependency of the benchmarks target
   add_custom_target(${test_name}.compile DEPENDS ${output_file})
   add_dependencies(benchmarks ${test_name}.compile)

endforeach()
