
add_executable(darma_hello_03 darma_hello.cc)
target_link_libraries(darma_hello_03 ${DARMA_BACKEND_LIBNAME})
add_test_for_example(darma_hello_03 ${CMAKE_CURRENT_BINARY_DIR}/darma_hello_03 --backend-n-ranks 8)

