
add_executable(publish_get_parallel publish_get_parallel.cc)
target_link_libraries(publish_get_parallel ${DARMA_BACKEND_LIBNAME})
add_test_for_example(publish_get_parallel ${CMAKE_CURRENT_BINARY_DIR}/publish_get_parallel "")
