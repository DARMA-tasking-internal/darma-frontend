
set(DARMA_KEY_HEADERS
  SSO_key.h
  SSO_key.impl.h
  SSO_key_fwd.h
  bytes_convert.h
  bytes_type_metadata.h
  key.h
  key.impl.h
  key_concept.h
  key_fwd.h
  raw_bytes.h
)

install (FILES ${DARMA_KEY_HEADERS} DESTINATION include/darma/key)

add_subdirectory(dependent_on/serialization)


