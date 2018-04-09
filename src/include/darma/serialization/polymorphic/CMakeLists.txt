
set(DARMA_SERIALIZATION_POLYMORPHIC_HEADERS
  polymorphic_serializable_object.h
  polymorphic_serialization_adapter.h
  registry.h
)

add_subdirectory(impl)

install (FILES ${DARMA_SERIALIZATION_POLYMORPHIC_HEADERS}
  DESTINATION include/darma/serialization/polymorphic
)

