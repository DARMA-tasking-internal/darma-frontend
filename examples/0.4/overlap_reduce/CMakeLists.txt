add_executable(overlap_reduce overlap_reduce.cc)
target_link_libraries(overlap_reduce ${DARMA_BACKEND_LIBNAME})

add_test_for_example(overlap_reduce_1proc ${CMAKE_CURRENT_BINARY_DIR}/overlap_reduce --ranks 1 --app-argv 10)
add_test_for_example(overlap_reduce_2proc ${CMAKE_CURRENT_BINARY_DIR}/overlap_reduce --ranks 2 --app-argv 20)
add_test_for_example(overlap_reduce_4proc ${CMAKE_CURRENT_BINARY_DIR}/overlap_reduce --ranks 4 --app-argv 20)


