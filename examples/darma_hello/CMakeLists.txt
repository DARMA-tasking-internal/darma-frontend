
add_executable(darma_hello darma_hello.cc)

if (IN_TREE_BACKEND)
target_link_libraries(darma_hello ${IN_TREE_BACKEND})
endif()

