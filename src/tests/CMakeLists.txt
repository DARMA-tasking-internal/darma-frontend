
include_directories( ${DARMA_FE_REPO_ROOT}/src/tests )
include_directories( ${DARMA_FE_REPO_ROOT}/src/include )

if (NOT GTEST_FOUND)
  message("It is recommended to define the prefix install dir for GoogleTest via -DGTEST_DIR=")
else()
  include_directories( ${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS} )
endif()

include (../../cmake-modules/SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

add_subdirectory(tinympl)
add_subdirectory(meta)
add_subdirectory(frontend_validation)
#add_subdirectory(frontend_compilation_benchmark)
add_subdirectory(serialization)

