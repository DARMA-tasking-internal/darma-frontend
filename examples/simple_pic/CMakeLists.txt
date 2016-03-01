
add_executable(run_pic simple_pic.cc)

if (IN_TREE_BACKEND)
target_link_libraries(run_pic ${IN_TREE_BACKEND})
endif()

