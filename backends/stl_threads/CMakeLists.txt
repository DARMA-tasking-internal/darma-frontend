set (threads_HEADERS
threads_backend.h
darma_types.h
)

set (threads_SOURCES 
threads_backend.cc
)

add_library( darma_stl_threads_backend SHARED ${threads_SOURCES} ${threads_HEADERS} )
install (TARGETS darma_stl_threads_backend DESTINATION lib)

