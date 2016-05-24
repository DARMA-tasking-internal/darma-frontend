
add_executable(initAndFinalize initAndFinalize.cc)
target_link_libraries(initAndFinalize ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__initAndFinalize ${CMAKE_CURRENT_BINARY_DIR}/initAndFinalize "")

add_executable(rankAndSize rankAndSize.cc)
target_link_libraries(rankAndSize ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__rankAndSize ${CMAKE_CURRENT_BINARY_DIR}/rankAndSize "")

add_executable(cw1 cw1.cc)
target_link_libraries(cw1 ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__cw1 ${CMAKE_CURRENT_BINARY_DIR}/cw1 "")

add_executable(initialAccess initialAccess.cc)
target_link_libraries(initialAccess ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__initialAccess ${CMAKE_CURRENT_BINARY_DIR}/initialAccess "")

add_executable(accessHandle1 accessHandle1.cc)
target_link_libraries(accessHandle1 ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__accessHandle1 ${CMAKE_CURRENT_BINARY_DIR}/accessHandle1 "")

add_executable(accessHandleArrowOperator accessHandleArrowOperator.cc)
target_link_libraries(accessHandleArrowOperator ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__accessHandleArrowOperator ${CMAKE_CURRENT_BINARY_DIR}/accessHandleArrowOperator "")

add_executable(publishAndReadAccess publishAndReadAccess.cc)
target_link_libraries(publishAndReadAccess ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__publishAndReadAccess ${CMAKE_CURRENT_BINARY_DIR}/publishAndReadAccess "")

add_executable(cwReadsArg cwReadsArg.cc)
target_link_libraries(cwReadsArg ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__cwReadsArg ${CMAKE_CURRENT_BINARY_DIR}/cwReadsArg "")

add_executable(version version.cc)
target_link_libraries(version ${DARMA_BACKEND_LIBNAME})
add_test_for_example(spec02individualFunctionalities__version ${CMAKE_CURRENT_BINARY_DIR}/version "")
