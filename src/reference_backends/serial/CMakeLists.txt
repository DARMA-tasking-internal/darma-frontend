project (DarmaSerialBackend)

set (DarmaSerialBackend_VERSION_MAJOR 0)
set (DarmaSerialBackend_VERSION_MINOR 2)
set (DarmaSerialBackend_VERSION_PATCH 0)
set (DarmaSerialBackend_VERSION
  ${DarmaSerialBackend_VERSION_MAJOR}.${DarmaSerialBackend_VERSION_MINOR}.${DarmaSerialBackend_VERSION_PATCH})

set (serial_HEADERS
serial_backend.h
darma_types.h
)

set (serial_SOURCES 
serial_backend.cc
)

include_directories( ${BACKENDS_ROOT}/serial )
add_library( darma_serial_backend SHARED ${serial_SOURCES} ${serial_HEADERS} )
install (TARGETS darma_serial_backend EXPORT DarmaSerialBackendDepends DESTINATION lib)
install (FILES darma_types.h DESTINATION include)

# Add all targets to be installed to the build-tree export set
export (TARGETS darma_serial_backend
  FILE "${PROJECT_BINARY_DIR}/DarmaSerialBackendDepends.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export (PACKAGE darma_serial_backend)

# Create the DarmaSerialBackendConfig.cmake and
# DarmaSerialBackendConfigVersion.cmake files
configure_file (cmake/DarmaSerialBackendConfig.cmake.in
  "${PROJECT_BINARY_DIR}/DarmaSerialBackendConfig.cmake" @ONLY)
configure_file (cmake/DarmaSerialBackendConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/DarmaSerialBackendConfigVersion.cmake" @ONLY)

# Install DarmaSerialBackendConfig.cmake and
# DarmaSerialBackendConfigVersion.cmake
install (FILES
  "${PROJECT_BINARY_DIR}/DarmaSerialBackendConfig.cmake"
  "${PROJECT_BINARY_DIR}/DarmaSerialBackendConfigVersion.cmake"
  DESTINATION cmake COMPONENT dev)

# Install the export set for use with the install-tree
install (EXPORT DarmaSerialBackendDepends DESTINATION cmake COMPONENT dev)
