
set(example_standard_args --backend-n-ranks 4)

add_executable(heat_1d_darma_simple heat_1d_darma_simple.cc)
target_link_libraries(heat_1d_darma_simple ${DARMA_BACKEND_LIBNAME})
add_test_for_example(heat_1d_darma_simple ${CMAKE_CURRENT_BINARY_DIR}/heat_1d_darma_simple "")

