add_executable(persistent_roots persistent_roots.cc)
target_link_libraries(persistent_roots ${DARMA_BACKEND_LIBNAME})
add_test_for_example(persistent_roots ${CMAKE_CURRENT_BINARY_DIR}/persistent_roots --ranks 8 100 15 5 100 3000)
