
add_executable(rank rank.cc)

if (IN_TREE_BACKEND)
target_link_libraries(rank ${IN_TREE_BACKEND})
endif()

