
set(DARMA_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_HEADERS
  list.h
  map.h
  pair.h
  set.h
  string.h
  tuple.h
  vector.h
)

install (FILES ${DARMA_SERIALIZATION_SERIALIZERS_STANDARD_LIBRARY_HEADERS}
  DESTINATION include/darma/serialization/serializers/standard_library
)


