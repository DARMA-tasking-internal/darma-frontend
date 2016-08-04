
#add_executable(test_detection test_detection.cc)
#add_executable(test_member_detector test_member_detector.cc)
#add_executable(test_splat_tuple test_splat_tuple.cc)
#add_executable(test_tuple_for_each_old test_tuple_for_each_old.cc)

if (NOT GTEST_DIR)
    message(FATAL_ERROR "Must define prefix install dir for GoogleTest via -DGTEST_DIR=")
endif()

include_directories( ${GTEST_DIR}/include )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")
include_directories( ${CMAKE_SOURCE_DIR}/src/tests/meta )

set(metatestfiles gtest_main.cc )
function(add_unit_test test_name)
    set(metatestfiles "${metatestfiles};${test_name}.cc" PARENT_SCOPE)
    add_executable(${test_name} ${test_name}.cc gtest_main.cc)
    target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
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
add_unit_test(test_any_convertible)

add_executable(run_all_meta_tests ${metatestfiles})
target_link_libraries(run_all_meta_tests ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
if (DARMA_COVERAGE)
    setup_target_for_coverage(meta_coverage run_all_meta_tests coverage)
endif()
