add_executable(simple_collection_remap simple_collection_remap.cc)
target_link_libraries(simple_collection_remap ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_remap_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_remap --ranks 1 --app-argv 10)
add_test_for_example(simple_collection_remap_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_remap --ranks 2 --app-argv 10)
add_test_for_example(simple_collection_remap_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_remap --ranks 4 --app-argv 10)
