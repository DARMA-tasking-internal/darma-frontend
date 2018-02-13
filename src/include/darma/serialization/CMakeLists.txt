
set(DARMA_SERIALIZATION_HEADERS
  direct_serialization.h
  nonintrusive.h
  pointer_reference_archive.h
  pointer_reference_handler.h
  pointer_reference_handler_fwd.h
  serialization_buffer.h
  serialization_fwd.h
  serialization_traits.h
  serialization_traits.impl.h
  serialization_traits_fwd.h
  simple_archive.h
  simple_handler.h
  simple_handler_fwd.h
)

install (FILES ${DARMA_SERIALIZATION_HEADERS} DESTINATION include/darma/serialization)

add_subdirectory(serializers)

