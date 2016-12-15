
include_directories( ${CMAKE_SOURCE_DIR}/src/tests )
include_directories( ${CMAKE_SOURCE_DIR}/src/include )

include (../../cmake-modules/SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

add_subdirectory(tinympl)
add_subdirectory(meta)
add_subdirectory(frontend_validation)

