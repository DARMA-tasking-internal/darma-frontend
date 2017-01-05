add_executable(fib_nokey fib_nokey.cc)
target_link_libraries(fib_nokey ${DARMA_BACKEND_LIBNAME})
add_test_for_example(fib_nokey ${CMAKE_CURRENT_BINARY_DIR}/fib_nokey --app-argv=10)
