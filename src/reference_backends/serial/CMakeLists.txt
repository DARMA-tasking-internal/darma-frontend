set (serial_HEADERS
serial_backend.h
darma_types.h
)

set (serial_SOURCES 
serial_backend.cc
)

include_directories( ${BACKENDS_ROOT}/serial )
add_library( darma_serial_backend SHARED ${serial_SOURCES} ${serial_HEADERS} )
install (TARGETS darma_serial_backend DESTINATION lib)
install (FILES darma_types.h DESTINATION include)

