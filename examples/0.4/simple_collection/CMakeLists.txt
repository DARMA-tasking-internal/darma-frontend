add_executable(simple_collection simple_collection.cc)
target_link_libraries(simple_collection ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection --ranks 1 --app-argv=10)
add_test_for_example(simple_collection_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection --ranks 2 --app-argv=10)
add_test_for_example(simple_collection_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection --ranks 4 --app-argv=10)
