
add_executable(single_rank single_rank.cc)

if (IN_TREE_BACKEND)
target_link_libraries(single_rank ${IN_TREE_BACKEND})
endif()

