
add_executable(single_rank_MC single_rank_MC.cc)
target_link_libraries(single_rank_MC ${DARMA_BACKEND_LIBNAME})
add_test(poisson_1d_thomasAlgorithm__single_rank_MC ${CMAKE_CURRENT_BINARY_DIR}/single_rank_MC ${example_standard_args})

