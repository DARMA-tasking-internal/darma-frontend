
include_directories( ${CMAKE_SOURCE_DIR}/src/tests )
include_directories( ${CMAKE_SOURCE_DIR}/src/include )
include_directories( ${CMAKE_SOURCE_DIR}/src/include/impl )

# Compiler-specific C++11 activation.
set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} -std=c++1y")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if (NOT (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8))
    message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.8 or greater.")
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else ()
  message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif ()

# nothing, for now
add_subdirectory(meta)
add_subdirectory(tinympl)


