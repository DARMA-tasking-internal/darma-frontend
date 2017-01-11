add_executable(simple_collection_unique_owner simple_collection_unique_owner.cc)
target_link_libraries(simple_collection_unique_owner ${DARMA_BACKEND_LIBNAME})
add_test_for_example(simple_collection_unique_owner ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_unique_owner --bf 100 --ranks 4 --app-argv=10)
