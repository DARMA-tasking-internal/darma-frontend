
add_executable(initAndFinalize initAndFinalize.cc)
target_link_libraries(initAndFinalize ${DARMA_BACKEND_LIBNAME})

add_executable(rankAndSize rankAndSize.cc)
target_link_libraries(rankAndSize ${DARMA_BACKEND_LIBNAME})

add_executable(cw1 cw1.cc)
target_link_libraries(cw1 ${DARMA_BACKEND_LIBNAME})

add_executable(initialAccess initialAccess.cc)
target_link_libraries(initialAccess ${DARMA_BACKEND_LIBNAME})

#add_executable(accessHandle1 accessHandle1.cc)
#target_link_libraries(accessHandle1 ${DARMA_BACKEND_LIBNAME})

add_executable(accessHandleArrowOperator accessHandleArrowOperator.cc)
target_link_libraries(accessHandleArrowOperator ${DARMA_BACKEND_LIBNAME})

add_executable(publishAndReadAccess publishAndReadAccess.cc)
target_link_libraries(publishAndReadAccess ${DARMA_BACKEND_LIBNAME})

add_executable(cwReadsArg cwReadsArg.cc)
target_link_libraries(cwReadsArg ${DARMA_BACKEND_LIBNAME})

add_executable(version version.cc)
target_link_libraries(version ${DARMA_BACKEND_LIBNAME})
