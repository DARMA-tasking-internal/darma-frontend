
add_executable(run stencil_1d.cc)
target_link_libraries(run ${DARMA_BACKEND_LIBNAME})
add_test_for_example(stencil_1d__darma_simple ${CMAKE_CURRENT_BINARY_DIR}/run "")

