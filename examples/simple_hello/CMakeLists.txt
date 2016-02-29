
add_executable(simple_hello simple_hello.cc)

if (IN_TREE_BACKEND)
target_link_libraries(simple_hello ${IN_TREE_BACKEND})
endif()

