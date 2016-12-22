add_executable(simple_collection_ordering simple_collection_ordering.cc)
target_link_libraries(simple_collection_ordering ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_ordering_1proc_10 ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_ordering --ranks 1 --app-argv 10)
add_test_for_example(simple_collection_ordering_2proc_10 ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_ordering --ranks 2 --app-argv 10)
add_test_for_example(simple_collection_ordering_4proc_10 ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_ordering --ranks 4 --app-argv 10)

add_test_for_example(simple_collection_ordering_1proc_20 ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_ordering --ranks 1 --app-argv 20)
add_test_for_example(simple_collection_ordering_2proc_20 ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_ordering --ranks 2 --app-argv 20)
add_test_for_example(simple_collection_ordering_4proc_20 ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_ordering --ranks 4 --app-argv 20)
