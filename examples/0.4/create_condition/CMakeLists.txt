
add_executable(ccond03 main.cc)
target_link_libraries(ccond03 ${DARMA_BACKEND_LIBNAME})
add_test_for_example(ccond03 ${CMAKE_CURRENT_BINARY_DIR}/ccond03 --backend-n-ranks 4)
