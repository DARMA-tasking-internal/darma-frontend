
set(DARMA_SERIALIZATION_POLYMORPHIC_HEADERS
  impl/polymorphic_serializable_object.impl.h
  polymorphic_serializable_object.h
  polymorphic_serialization_adapter.h
  registry.h
)

install (FILES ${DARMA_SERIALIZATION_POLYMORPHIC_HEADERS}
  DESTINATION include/darma/serialization/serializers
)

