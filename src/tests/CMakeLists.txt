
include_directories( ${CMAKE_SOURCE_DIR}/src/tests )
include_directories( ${CMAKE_SOURCE_DIR}/src/include )
include_directories( ${CMAKE_SOURCE_DIR}/src/include/impl )

include (../../cmake-modules/SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

# nothing, for now
add_subdirectory(tinympl)
if (GTEST_DIR)
add_subdirectory(meta)
add_subdirectory(frontend_validation)
endif()


