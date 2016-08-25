project (DarmaThreadsBackend)

set (DarmaThreadsBackend_VERSION_MAJOR 0)
set (DarmaThreadsBackend_VERSION_MINOR 2)
set (DarmaThreadsBackend_VERSION_PATCH 0)
set (DarmaThreadsBackend_VERSION
  ${DarmaThreadsBackend_VERSION_MAJOR}.${DarmaThreadsBackend_VERSION_MINOR}.${DarmaThreadsBackend_VERSION_PATCH})

set (threads_HEADERS
darma_types.h
)

set (threads_SOURCES 
threads.cc
)

include_directories( ${BACKENDS_ROOT}/threads )
add_library( darma_threads_backend SHARED ${threads_SOURCES} ${threads_HEADERS} )
if (GTEST_DIR)
add_library( darma_threads_backend_with_mock_frontend
        SHARED ${threads_SOURCES} ${threads_HEADERS} )
#target_compile_definitions(darma_threads_backend_with_mock_frontend PUBLIC DARMA_HAS_FRONTEND_TYPES_H=0)
target_include_directories(darma_threads_backend_with_mock_frontend PUBLIC ${GTEST_DIR}/include)
find_library(GMOCK_LIB_PATH gmock ${GTEST_DIR}/lib)
find_library(GTEST_LIB_PATH gtest ${GTEST_DIR}/lib)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")
    #link_directories(${GTEST_DIR}/lib)
target_link_libraries(darma_threads_backend_with_mock_frontend ${GMOCK_LIB_PATH} ${GTEST_LIB_PATH} pthread)
endif()
install (TARGETS darma_threads_backend EXPORT DarmaThreadsBackendDepends DESTINATION lib)
install (FILES darma_types.h DESTINATION include)

# Add all targets to be installed to the build-tree export set
export (TARGETS darma_threads_backend
  FILE "${PROJECT_BINARY_DIR}/DarmaThreadsBackendDepends.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export (PACKAGE darma_threads_backend)

# Create the DarmaThreadsBackendConfig.cmake and
# DarmaThreadsBackendConfigVersion.cmake files
configure_file (cmake/DarmaThreadsBackendConfig.cmake.in
  "${PROJECT_BINARY_DIR}/DarmaThreadsBackendConfig.cmake" @ONLY)
configure_file (cmake/DarmaThreadsBackendConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/DarmaThreadsBackendConfigVersion.cmake" @ONLY)

# Install DarmaThreadsBackendConfig.cmake and
# DarmaThreadsBackendConfigVersion.cmake
install (FILES
  "${PROJECT_BINARY_DIR}/DarmaThreadsBackendConfig.cmake"
  "${PROJECT_BINARY_DIR}/DarmaThreadsBackendConfigVersion.cmake"
  DESTINATION cmake COMPONENT dev)

# Install the export set for use with the install-tree
install (EXPORT DarmaThreadsBackendDepends DESTINATION cmake COMPONENT dev)
