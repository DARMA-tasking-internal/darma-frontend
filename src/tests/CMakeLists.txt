
include_directories( ${DARMA_FE_REPO_ROOT}/src/tests )
include_directories( ${DARMA_FE_REPO_ROOT}/src/include )

include (../../cmake-modules/SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

add_subdirectory(tinympl)
add_subdirectory(meta)
add_subdirectory(frontend_validation)
add_subdirectory(serialization)

