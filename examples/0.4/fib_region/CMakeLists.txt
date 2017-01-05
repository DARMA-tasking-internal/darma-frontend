add_executable(fib_region fib_region.cc)
target_link_libraries(fib_region ${DARMA_BACKEND_LIBNAME})
add_test_for_example(fib_region ${CMAKE_CURRENT_BINARY_DIR}/fib_region --ranks 8 --app-argv 10)
