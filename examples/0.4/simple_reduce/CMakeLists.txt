add_executable(simple_reduce simple_reduce.cc)
target_link_libraries(simple_reduce ${DARMA_BACKEND_LIBNAME})
add_test_for_example(simple_reduce ${CMAKE_CURRENT_BINARY_DIR}/simple_reduce --ranks 8 20)
