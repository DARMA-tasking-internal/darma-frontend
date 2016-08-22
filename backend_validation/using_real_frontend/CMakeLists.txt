cmake_minimum_required (VERSION 2.8)

enable_testing()

set (validation_HEADERS
helpers.h
test_backend.h)

install (FILES ${validation_HEADERS} DESTINATION
  include/darma/backend_validation/using_real_frontend)

include_directories( ${VALIDATION_SRC_ROOT}/using_real_frontend )
include_directories( ${VALIDATION_BIN_ROOT}/using_real_frontend )

set(backendtestfiles main.cc )
function(add_unit_test test_name)
set(backendtestfiles "${backendtestfiles};${test_name}.cc" PARENT_SCOPE)
add_executable(${test_name} ${test_name}.cc main.cc)
target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "--gtest_repeat=3")
set_tests_properties(${test_name} PROPERTIES TIMEOUT 20
  FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is;WARNING"
  PASS_REGULAR_EXPRESSION "PASSED")
endfunction()
#
add_unit_test(test_init_be)
add_unit_test(test_flows_be)
add_unit_test(test_create_work_be)
add_unit_test(test_publish_be)
add_unit_test(test_create_condition_be)

add_executable(run_all_coupled_backend_tests ${backendtestfiles})
set_target_properties(run_all_coupled_backend_tests PROPERTIES COMPILE_DEFINITIONS "DARMA_COVERAGE")
target_link_libraries(run_all_coupled_backend_tests ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)

if (DARMA_COVERAGE)
setup_target_for_backend_coverage(backend_coupled_coverage run_all_coupled_backend_tests coverage_be_coupled)
endif()
