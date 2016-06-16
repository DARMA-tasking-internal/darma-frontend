set (main_HEADERS
frontend.h)

set (app_HEADERS
app/access_handle.h
app/create_condition.h
app/create_work.h
app/darma.h
app/initial_access.h
app/read_access.h)

set (backend_HEADERS
backend/flow.h
backend/runtime.h
backend/types.h)

set (default_HEADERS
defaults/darma_main.h
defaults/key.h
defaults/pointers.h)

set (frontend_HEADERS
frontend/frontend_fwd.h
frontend/handle.h
frontend/publication_details.h
frontend/serialization_manager.h
frontend/task.h
frontend/types.h
frontend/use.h)

set (frontend_detail_HEADERS
frontend/detail/crtp_impl.h
frontend/detail/task.crtp_impl.h)


install (FILES ${main_HEADERS} DESTINATION include/darma/interface)
install (FILES ${app_HEADERS} DESTINATION include/darma/interface/app)
install (FILES ${backend_HEADERS} DESTINATION include/darma/interface/backend)
install (FILES ${default_HEADERS} DESTINATION include/darma/interface/defaults)
install (FILES ${frontend_HEADERS} DESTINATION include/darma/interface/frontend)
install (FILES ${frontend_detail_HEADERS} DESTINATION include/darma/interface/frontend/detail)
