
add_executable(single_rank_MC single_rank_MC.cc)

if (IN_TREE_BACKEND)
target_link_libraries(single_rank_MC ${IN_TREE_BACKEND})
endif()

