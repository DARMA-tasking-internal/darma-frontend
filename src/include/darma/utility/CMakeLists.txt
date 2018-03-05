
set(DARMA_UTILITY_HEADERS
  compatibility.h
  compressed_pair.h
  config.h
  constexpr_if.h
  demangle.h
  macros.h
  managed_swap_storage.h
  not_a_type.h
  optional_boolean.h
  safe_static_cast.h
  static_assertions.h
)

install (FILES ${DARMA_UTILITY_HEADERS} DESTINATION include/darma/utility)

################################################################################
# Checks for configuration details
################################################################################

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
  config-generated.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config-generated.h
)
install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/config-generated.h
  DESTINATION include/darma/impl
)

################################################################################

