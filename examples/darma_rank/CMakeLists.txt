
add_executable(rank rank.cc)
target_link_libraries(rank ${DARMA_BACKEND_LIBNAME})
add_test(darma_rank ${CMAKE_CURRENT_BINARY_DIR}/rank ${example_standard_args})

