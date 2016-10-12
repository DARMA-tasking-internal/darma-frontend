add_executable(square_roots square_roots.cc)
target_link_libraries(square_roots ${DARMA_BACKEND_LIBNAME})
add_test_for_example(square_roots ${CMAKE_CURRENT_BINARY_DIR}/square_roots --ranks 8 100 15 5 100 3000)
