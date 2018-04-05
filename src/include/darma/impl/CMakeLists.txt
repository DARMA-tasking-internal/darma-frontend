
FILE(GLOB impl_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
FILE(GLOB impl_access_handle_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/access_handle/*.h")
FILE(GLOB impl_array_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/array/*.h")
FILE(GLOB impl_async_accessible_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/async_accessible/*.h")
FILE(GLOB impl_capture_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/capture/*.h")
FILE(GLOB impl_collective_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/collective/*.h")
FILE(GLOB impl_create_work_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/create_work/*.h")
FILE(GLOB impl_index_range_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/index_range/*.h")
FILE(GLOB impl_key_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/key/*.h")
FILE(GLOB impl_keyword_arguments_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "../keyword_arguments/*.h")
FILE(GLOB impl_meta_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/meta/*.h")
FILE(GLOB impl_oo_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/oo/*.h")
FILE(GLOB impl_runnable_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/runnable/*.h")
FILE(GLOB impl_serialization_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization/*.h")
FILE(GLOB impl_task_collection_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/task_collection/*.h")
FILE(GLOB impl_task_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/task/*.h")
FILE(GLOB impl_task_collection_impl_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/task_collection/impl/*.h")
FILE(GLOB impl_use_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/use/*.h")
FILE(GLOB impl_util_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/util/*.h")
FILE(GLOB impl_mpi_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/mpi/*.h")

install (FILES ${impl_HEADERS} DESTINATION include/darma/impl)
install (FILES ${impl_access_handle_HEADERS} DESTINATION include/darma/impl/access_handle)
install (FILES ${impl_array_HEADERS} DESTINATION include/darma/impl/array)
install (FILES ${impl_async_accessible_HEADERS} DESTINATION include/darma/impl/async_accessible)
install (FILES ${impl_capture_HEADERS} DESTINATION include/darma/impl/capture)
install (FILES ${impl_collective_HEADERS} DESTINATION include/darma/impl/collective)
install (FILES ${impl_create_work_HEADERS} DESTINATION include/darma/impl/create_work)
install (FILES ${impl_index_range_HEADERS} DESTINATION include/darma/impl/index_range)
install (FILES ${impl_key_HEADERS} DESTINATION include/darma/impl/key)
install (FILES ${impl_keyword_arguments_HEADERS} DESTINATION include/darma/keyword_arguments)
install (FILES ${impl_meta_HEADERS} DESTINATION include/darma/impl/meta)
install (FILES ${impl_oo_HEADERS} DESTINATION include/darma/impl/oo)
install (FILES ${impl_runnable_HEADERS} DESTINATION include/darma/impl/runnable)
install (FILES ${impl_serialization_HEADERS} DESTINATION include/darma/impl/serialization)
install (FILES ${impl_task_HEADERS} DESTINATION include/darma/impl/task)
install (FILES ${impl_task_collection_HEADERS} DESTINATION include/darma/impl/task_collection)
install (FILES ${impl_task_collection_impl_HEADERS} DESTINATION include/darma/impl/task_collection/impl)
install (FILES ${impl_use_HEADERS} DESTINATION include/darma/impl/use)
install (FILES ${impl_util_HEADERS} DESTINATION include/darma/impl/util)
install (FILES ${impl_mpi_HEADERS} DESTINATION include/darma/impl/mpi)
