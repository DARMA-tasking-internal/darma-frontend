
FILE(GLOB impl_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
FILE(GLOB impl_array_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/array/*.h")
FILE(GLOB impl_collective_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/collective/*.h")
FILE(GLOB impl_util_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/access_handle/*.h")
FILE(GLOB impl_index_range_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/index_range/*.h")
FILE(GLOB impl_key_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/key/*.h")
FILE(GLOB impl_keyword_arguments_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/keyword_arguments/*.h")
FILE(GLOB impl_meta_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/meta/*.h")
FILE(GLOB impl_oo_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/oo/*.h")
FILE(GLOB impl_runnable_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/runnable/*.h")
FILE(GLOB impl_serialization_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization/*.h")
FILE(GLOB impl_task_collection_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/task_collection/*.h")
FILE(GLOB impl_task_collection_impl_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/task_collection/impl/*.h")
FILE(GLOB impl_util_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/util/*.h")

install (FILES ${impl_HEADERS} DESTINATION include/darma/impl)
install (FILES ${impl_array_HEADERS} DESTINATION include/darma/impl/array)
install (FILES ${impl_collective_HEADERS} DESTINATION include/darma/impl/collective)
install (FILES ${impl_index_range_HEADERS} DESTINATION include/darma/impl/index_range)
install (FILES ${impl_key_HEADERS} DESTINATION include/darma/impl/key)
install (FILES ${impl_keyword_arguments_HEADERS} DESTINATION include/darma/impl/keyword_arguments)
install (FILES ${impl_meta_HEADERS} DESTINATION include/darma/impl/meta)
install (FILES ${impl_oo_HEADERS} DESTINATION include/darma/impl/oo)
install (FILES ${impl_runnable_HEADERS} DESTINATION include/darma/impl/runnable)
install (FILES ${impl_serialization_HEADERS} DESTINATION include/darma/impl/serialization)
install (FILES ${impl_task_collection_HEADERS} DESTINATION include/darma/impl/task_collection)
install (FILES ${impl_task_collection_impl_HEADERS} DESTINATION include/darma/impl/task_collection/impl)
install (FILES ${impl_util_HEADERS} DESTINATION include/darma/impl/util)

################################################################################
# Check for available attributes
set(CMAKE_REQUIRED_FLAGS_TMP ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_TMP} -Werror=attributes -std=c++14 ${CMAKE_CXX_FLAGS}")

check_cxx_source_compiles("
[[nonnull(1,2)]]
extern void* my_memcpy(void *dest, const void *src)
int main() { }
" DARMA_HAVE_ATTRIBUTE_NONNULL)

check_cxx_source_compiles("
#include <cassert>
[[ deprecated ]] void* my_memcpy(void *dest, const void *src);
int main() { }
" DARMA_HAVE_ATTRIBUTE_DEPRECATED)

check_cxx_source_compiles("
[[ deprecated(\"reason\") ]]
void* my_memcpy(void *dest, const void *src);
int main() { }
" DARMA_HAVE_ATTRIBUTE_DEPRECATED_WITH_MESSAGE)

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_TMP}")
# end of attribute checks
################################################################################

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config-generated.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config-generated.h
)
install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/config-generated.h
  DESTINATION include/darma/impl
)
