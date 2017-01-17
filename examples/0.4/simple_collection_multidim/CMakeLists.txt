add_executable(simple_collection_multidim simple_collection_multidim.cc)
target_link_libraries(simple_collection_multidim ${DARMA_BACKEND_LIBNAME})

add_test_for_example(simple_collection_multidim_1proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_multidim --ranks 1 --app-argv 8 6)
add_test_for_example(simple_collection_multidim_2proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_multidim --ranks 2 --app-argv 8 6)
add_test_for_example(simple_collection_multidim_4proc ${CMAKE_CURRENT_BINARY_DIR}/simple_collection_multidim --ranks 4 --app-argv 8 6)
