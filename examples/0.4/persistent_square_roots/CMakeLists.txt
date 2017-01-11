add_executable(persistent_roots persistent_roots.cc)
target_link_libraries(persistent_roots ${DARMA_BACKEND_LIBNAME})

add_test_for_example(persistent_roots_1proc ${CMAKE_CURRENT_BINARY_DIR}/persistent_roots --ranks 1 --app-argv 100 15 5 100 3000)
add_test_for_example(persistent_roots_2proc ${CMAKE_CURRENT_BINARY_DIR}/persistent_roots --ranks 2 --app-argv 100 15 5 100 3000)
add_test_for_example(persistent_roots_4proc ${CMAKE_CURRENT_BINARY_DIR}/persistent_roots --ranks 4 --app-argv 100 15 5 100 3000)
