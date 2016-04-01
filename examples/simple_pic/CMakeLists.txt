
add_executable(run_pic simple_pic.cc)
target_link_libraries(run_pic ${DARMA_BACKEND_LIBNAME})
add_test(simple_pic ${CMAKE_CURRENT_BINARY_DIR}/run_pic ${example_standard_args})

