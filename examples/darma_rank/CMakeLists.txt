
add_executable(rank rank.cc)
target_link_libraries(rank ${DARMA_BACKEND_LIBNAME})
add_test_for_example(darma_rank ${CMAKE_CURRENT_BINARY_DIR}/rank "")

