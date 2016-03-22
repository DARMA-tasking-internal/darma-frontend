
if (NOT GTEST_DIR)
message(FATAL_ERROR "Must define prefix install dir for GoogleTest via -DGTEST_DIR=")
endif()

include_directories( ${GTEST_DIR}/include )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")
include_directories( ${CMAKE_SOURCE_DIR}/src/tests/frontend_validation )

function(add_unit_test test_name)
add_executable(${test_name} ${test_name}.cc)
target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "")
set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
  FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is"
  PASS_REGULAR_EXPRESSION "PASSED")
endfunction()

add_unit_test(test_read_access)
add_unit_test(test_initial_access)
add_unit_test(test_create_work)
add_unit_test(test_access_handle)

