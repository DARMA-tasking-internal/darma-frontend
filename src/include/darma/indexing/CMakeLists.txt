
set(DARMA_INDEXING_HEADERS
  basic_range_1d.h
  polymorphic_index_range.h
)

install (FILES ${DARMA_INDEXING_HEADERS} DESTINATION include/darma/indexing)

add_subdirectory(abstract)

