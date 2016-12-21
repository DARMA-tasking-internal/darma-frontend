add_executable(simple_collection_fetch simple_collection_fetch.cc)
target_link_libraries(simple_collection_fetch ${DARMA_BACKEND_LIBNAME})
add_test_for_example(simple_collection_fetch ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_fetch --bf 100 --ranks 4 --app-argv "10 100")
