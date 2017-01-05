add_executable(square_roots square_roots.cc)
target_link_libraries(square_roots ${DARMA_BACKEND_LIBNAME})

add_test_for_example(square_roots_1proc ${CMAKE_CURRENT_BINARY_DIR}/square_roots --ranks 1 --app-argv 100 15 5 3000)
add_test_for_example(square_roots_2proc ${CMAKE_CURRENT_BINARY_DIR}/square_roots --ranks 2 --app-argv 100 15 5 3000)
add_test_for_example(square_roots_4proc ${CMAKE_CURRENT_BINARY_DIR}/square_roots --ranks 4 --app-argv 100 15 5 3000)
