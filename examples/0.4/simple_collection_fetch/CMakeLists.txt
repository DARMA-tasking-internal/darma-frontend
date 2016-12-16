add_executable(simple_collection_fetch simple_collection_fetch.cc)
target_link_libraries(simple_collection_fetch ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_fetch_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_fetch --ranks 1 10 100)
add_test_for_example(simple_collection_fetch_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_fetch --ranks 2 10 100)
add_test_for_example(simple_collection_fetch_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_fetch --ranks 4 10 100)
