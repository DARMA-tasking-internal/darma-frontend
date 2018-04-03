
set(DARMA_SERIALIZATION_SERIALIZERS_HEADERS
  all.h
  arithmetic_types.h
  array.h
  c_string.h
  const.h
  enum.h
)

install (FILES ${DARMA_SERIALIZATION_SERIALIZERS_HEADERS}
  DESTINATION include/darma/serialization/serializers
)

add_subdirectory(standard_library)

