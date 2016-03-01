set (mock_HEADERS
mock_backend.h
darma_types.h
)

set (mock_SOURCES 
mock_backend.cc
)

include_directories( ${BACKENDS_ROOT}/mock )
add_library( darma_mock_backend SHARED ${mock_SOURCES} ${mock_HEADERS} )
install (TARGETS darma_mock_backend DESTINATION lib)
install (FILES darma_types.h DESTINATION include)

