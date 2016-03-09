
add_executable(heat_1d_darma_simple heat_1d_darma_simple.cc)

if (IN_TREE_BACKEND)
target_link_libraries(heat_1d_darma_simple ${IN_TREE_BACKEND})
endif()

