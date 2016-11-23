add_executable(jacobi_2d jacobi_2d.cc)
target_link_libraries(jacobi_2d ${DARMA_BACKEND_LIBNAME})
add_test_for_example(jacobi_2d ${CMAKE_CURRENT_BINARY_DIR}/jacobi_2d --ranks 8 4 4 100 100 10)
