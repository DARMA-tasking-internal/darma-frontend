add_executable(fib fib.cc)
target_link_libraries(fib ${DARMA_BACKEND_LIBNAME})
add_test_for_example(fib ${CMAKE_CURRENT_BINARY_DIR}/fib 10)
