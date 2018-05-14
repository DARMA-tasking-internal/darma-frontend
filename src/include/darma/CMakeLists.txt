add_subdirectory(keyword_arguments)
add_subdirectory(key)
add_subdirectory(interface)
add_subdirectory(serialization)
add_subdirectory(impl)

################################################################################
#------------------------------------------------------------------------------#
# darma::utility interface library                                             #
#------------------------------------------------------------------------------#

add_library(utility INTERFACE)
add_library(darma::utility ALIAS utility)

target_include_directories(utility INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/include>
  $<INSTALL_INTERFACE:include>
  )
target_compile_features(utility INTERFACE cxx_std_14)

install(TARGETS utility EXPORT darmaUtilityTargets
  INCLUDES DESTINATION include
)

install(EXPORT darmaUtilityTargets
  FILE darmaUtilityTargets.cmake
  NAMESPACE darma::
  DESTINATION cmake
)

export(TARGETS utility
  NAMESPACE darma::
  FILE darmaUtilityTargets.cmake
)

#-------------------------------------------------------------------------------
# Check for available attributes
set(CMAKE_REQUIRED_FLAGS_TMP ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_TMP} -Werror=attributes -std=c++14 ${CMAKE_CXX_FLAGS}")

check_cxx_source_compiles("
[[ gnu::nonnull(1,2) ]]
extern void* my_memcpy(void *dest, const void *src);
int main() { }
"
  DARMA_HAVE_ATTRIBUTE_NONNULL
)

check_cxx_source_compiles("
[[ deprecated ]] void* my_memcpy(void *dest, const void *src);
int main() { }
"
  DARMA_HAVE_ATTRIBUTE_DEPRECATED
)

check_cxx_source_compiles("
[[ deprecated(\"reason\") ]]
void* my_memcpy(void *dest, const void *src);
int main() { }
"
  DARMA_HAVE_ATTRIBUTE_DEPRECATED_WITH_MESSAGE
)

check_cxx_source_compiles("
[[ gnu::always_inline ]] inline
int foo(int a) { return a + 5; }
int main() { }
"
  DARMA_HAVE_ATTRIBUTE_ALWAYS_INLINE
)

check_cxx_source_compiles("
[[ gnu::flatten ]]
int foo(int a) { return a + 5; }
int main() { }
"
  DARMA_HAVE_ATTRIBUTE_FLATTEN
)

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_TMP}")
# end of attribute checks
#-------------------------------------------------------------------------------


check_cxx_source_compiles("
#include <cxxabi.h>
int main() { }
"
  DARMA_HAVE_CXXABI_H
)


#-------------------------------------------------------------------------------
# Generate the configuration header

configure_file(
  utility/config-generated.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/utility/config-generated.h
)
install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/utility/config-generated.h
  DESTINATION include/darma/utility
)
target_compile_definitions(utility INTERFACE DARMA_CONFIG_GENERATED_H_AVAILABLE)

#-------------------------------------------------------------------------------

install(DIRECTORY utility DESTINATION include/darma FILES_MATCHING PATTERN "*.h")

################################################################################
