
#add_executable(test_detection test_detection.cc)
#add_executable(test_member_detector test_member_detector.cc)
#add_executable(test_splat_tuple test_splat_tuple.cc)
#add_executable(test_tuple_for_each_old test_tuple_for_each_old.cc)

if (NOT GTEST_FOUND)
  message("It is recommended to define the prefix install dir for GoogleTest via -DGTEST_DIR=")
else()
  include_directories( ${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS} )
endif()

find_package ( Threads REQUIRED )

include_directories( ${DARMA_FE_REPO_ROOT}/src/tests/meta )
function(add_unit_test test_name)
    set(metatestfiles "${metatestfiles};${test_name}.cc" PARENT_SCOPE)
    add_executable(${test_name} ${test_name}.cc gtest_main.cc)
    # GMock links to GTest, so all we need is GMock
    target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} ${GMOCK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "")
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
            FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is"
            PASS_REGULAR_EXPRESSION "PASSED")
endfunction()

add_unit_test(test_tuple_for_each)
add_unit_test(test_splat_tuple)
add_unit_test(test_tuple_zip)
add_unit_test(test_kwargs)
add_unit_test(test_callable_traits)
add_unit_test(test_compressed_pair)
add_unit_test(test_is_contiguous)
add_unit_test(test_is_iterator)
add_unit_test(test_any_convertible)

add_executable(run_all_meta_tests ${metatestfiles})
target_link_libraries(run_all_meta_tests ${DARMA_BACKEND_LIBNAME} ${GMOCK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
if (DARMA_COVERAGE)
    setup_target_for_coverage(meta_coverage run_all_meta_tests coverage)
endif()
