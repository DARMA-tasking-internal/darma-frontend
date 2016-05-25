
add_executable(darma_hello darma_hello.cc)
target_link_libraries(darma_hello ${DARMA_BACKEND_LIBNAME})
add_test_for_example(darma_hello ${CMAKE_CURRENT_BINARY_DIR}/darma_hello --backend-n-ranks 8)

