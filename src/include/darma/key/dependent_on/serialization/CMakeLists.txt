
set(DARMA_KEY_DEPENDENT_ON_SERIALIZATION_HEADERS
  SSO_key_serialization.h
  bytes_type_metadata_serialization.h
  raw_bytes_serialization.h
  key_serialization.h
)

install (FILES ${DARMA_KEY_DEPENDENT_ON_SERIALIZATION_HEADERS} DESTINATION include/darma/key/dependent_on/serialization)


