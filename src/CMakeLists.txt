include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/include )

add_subdirectory(include)

if (DARMA_FE_ENABLE_TESTS)
  add_subdirectory(tests)
endif()
