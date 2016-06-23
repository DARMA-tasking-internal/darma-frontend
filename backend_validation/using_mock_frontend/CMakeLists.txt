cmake_minimum_required (VERSION 2.8)

project (DarmaBackendValidation)
# The version number.
set (DarmaBackendValidation_VERSION_MAJOR 0)
set (DarmaBackendValidation_VERSION_MINOR 2)

enable_testing()

set (validation_HEADERS
frontend_types.h
helpers.h
main.h
mock_frontend.h
test_backend.h)

install (FILES ${validation_HEADERS} DESTINATION
  include/darma/backend_validation/using_mock_frontend)

# Let the compiler know that we have a custom frontend_types.h that replaces defines MockTask
# as concrete_task_t
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDARMA_HAS_FRONTEND_TYPES_H=1")

set(DARMA_BACKEND_LIBNAME ${DARMA_BACKEND_LIBNAME}_with_mock_frontend)

include_directories( ${VALIDATION_SRC_ROOT}/using_mock_frontend )
include_directories( ${VALIDATION_BIN_ROOT}/using_mock_frontend )

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
add_unit_test(test_register_task)

add_executable(run_all_isolated_backend_tests ${backendtestfiles})
target_link_libraries(run_all_isolated_backend_tests ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
