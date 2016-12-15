add_executable(sequence_reduce sequence_reduce.cc)
target_link_libraries(sequence_reduce ${DARMA_BACKEND_LIBNAME})

add_test_for_example(sequence_reduce_1proc ${CMAKE_CURRENT_BINARY_DIR}/sequence_reduce --ranks 1 25)
add_test_for_example(sequence_reduce_2proc ${CMAKE_CURRENT_BINARY_DIR}/sequence_reduce --ranks 2 25)
add_test_for_example(sequence_reduce_4proc ${CMAKE_CURRENT_BINARY_DIR}/sequence_reduce --ranks 4 25)
