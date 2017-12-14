
if (NOT GTEST_DIR)
  message("It is recommended to define the prefix install dir for GoogleTest via -DGTEST_DIR=")
else()
  include_directories( ${GTEST_DIR}/include )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")
endif()

find_package ( Threads REQUIRED )

include_directories( ${DARMA_FE_REPO_ROOT}/src/tests/serialization )
function(add_serialization_test test_name)
set(serializationtestfiles "${serializationtestfiles};${test_name}.cc" PARENT_SCOPE)
add_executable(${test_name} ${test_name}.cc gtest_main.cc)
target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock ${CMAKE_THREAD_LIBS_INIT})
add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "")
set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
  FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is"
  PASS_REGULAR_EXPRESSION "PASSED")
endfunction()

add_serialization_test(test_simple_arithmetic_types)
add_serialization_test(test_simple_std_pair)
add_serialization_test(test_simple_std_map)
add_serialization_test(test_simple_std_string)

add_executable(run_all_serialization_tests ${serializationtestfiles} gtest_main.cc)

target_link_libraries(run_all_serialization_tests gtest gmock ${CMAKE_THREAD_LIBS_INIT})
if (DARMA_COVERAGE)
setup_target_for_coverage(serialization_coverage run_all_serialization_tests coverage)
endif()
