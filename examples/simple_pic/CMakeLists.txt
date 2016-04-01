
add_executable(run_pic simple_pic.cc)
target_link_libraries(run_pic ${DARMA_BACKEND_LIBNAME})
add_test_for_example(simple_pic ${CMAKE_CURRENT_BINARY_DIR}/run_pic "")

