add_executable(simple_collection_iter simple_collection_iter.cc)
target_link_libraries(simple_collection_iter ${DARMA_BACKEND_LIBNAME})
add_test_for_example(simple_collection_iter ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_iter --bf 100 --ranks 4 --app-argv "10 100")
