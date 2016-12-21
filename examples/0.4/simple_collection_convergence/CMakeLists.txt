add_executable(simple_collection_convergence simple_collection_convergence.cc)
target_link_libraries(simple_collection_convergence ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_convergence_6_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 1 6 10000)
add_test_for_example(simple_collection_convergence_6_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 2 6 10000)
add_test_for_example(simple_collection_convergence_6_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 4 6 10000)

add_test_for_example(simple_collection_convergence_10_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 1 10 10000)
add_test_for_example(simple_collection_convergence_10_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 2 10 10000)
add_test_for_example(simple_collection_convergence_10_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 4 10 10000)

add_test_for_example(simple_collection_convergence_20_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 1 20 10000)
add_test_for_example(simple_collection_convergence_20_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 2 20 10000)
add_test_for_example(simple_collection_convergence_20_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_convergence --ranks 4 20 10000)
