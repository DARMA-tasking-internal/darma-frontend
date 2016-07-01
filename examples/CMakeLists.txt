cmake_minimum_required (VERSION 2.8)

project (DarmaExamples)
# The version number.
set (DarmaExamples_VERSION_MAJOR 0)
set (DarmaExamples_VERSION_MINOR 2)

enable_testing()

include (../SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

add_subdirectory(0.2)
add_subdirectory(0.3)
