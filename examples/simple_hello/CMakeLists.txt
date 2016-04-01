
add_executable(simple_hello simple_hello.cc)
target_link_libraries(simple_hello ${DARMA_BACKEND_LIBNAME})
add_test(simple_hello ${CMAKE_CURRENT_BINARY_DIR}/simple_hello ${example_standard_args})

