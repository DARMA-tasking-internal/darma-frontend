add_executable(simple_collection_iter simple_collection_iter.cc)
target_link_libraries(simple_collection_iter ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_iter_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_iter --ranks 1 --app-argv "10 100")
add_test_for_example(simple_collection_iter_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_iter --ranks 2 --app-argv "10 100")
add_test_for_example(simple_collection_iter_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_iter --ranks 4 --app-argv "10 100")
