
set (app_HEADERS
app/access_handle.h
app/create_work.h
app/initial_access.h
app/read_access.h
app/darma.h)

set (backend_HEADERS
backend/data_block.h
backend/runtime.h
backend/types.h)

set (default_HEADERS
defaults/darma_main.h
defaults/key.h
defaults/pointers.h
defaults/version.h)

set (frontend_HEADERS
frontend/dependency_handle.h
frontend/reduce_op.h
frontend/serialization_manager.h
frontend/task.h
frontend/types.h)


install (FILES ${app_HEADERS} DESTINATION include/darma/interface/app)
install (FILES ${backend_HEADERS} DESTINATION include/darma/interface/backend)
install (FILES ${default_HEADERS} DESTINATION include/darma/interface/defaults)
install (FILES ${frontend_HEADERS} DESTINATION include/darma/interface/frontend)

