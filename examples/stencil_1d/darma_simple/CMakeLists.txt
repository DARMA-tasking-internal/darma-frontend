
add_executable(run stencil_1d.cc)

if (IN_TREE_BACKEND)
target_link_libraries(run ${IN_TREE_BACKEND})
endif()

