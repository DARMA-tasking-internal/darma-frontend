add_executable(heat_1d_darma_simple_03 main.cc)
target_link_libraries(heat_1d_darma_simple_03 ${DARMA_BACKEND_LIBNAME})
add_test_for_example(heat_1d_darma_simple_03 ${CMAKE_CURRENT_BINARY_DIR}/heat_1d_darma_simple_03 --backend-n-ranks 4)

