
FILE(GLOB main_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

FILE(GLOB app_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/*.h")

FILE(GLOB app_key_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/keyword_arguments/*.h")

FILE(GLOB backend_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/backend/*.h")

FILE(GLOB default_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/defaults/*.h")

FILE(GLOB frontend_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/*.h")

FILE(GLOB frontend_detail_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/detail/*.h")

FILE(GLOB frontend_types_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/types/*.h")

install (FILES ${main_HEADERS} DESTINATION include/darma/interface)
install (FILES ${app_HEADERS} DESTINATION include/darma/interface/app)
install (FILES ${backend_HEADERS} DESTINATION include/darma/interface/backend)
install (FILES ${default_HEADERS} DESTINATION include/darma/interface/defaults)
install (FILES ${frontend_HEADERS} DESTINATION include/darma/interface/frontend)
install (FILES ${frontend_detail_HEADERS} DESTINATION include/darma/interface/frontend/detail)
install (FILES ${app_key_HEADERS} DESTINATION include/darma/interface/app/keyword_arguments)
install (FILES ${frontend_types_HEADERS} DESTINATION include/darma/interface/frontend/types)



