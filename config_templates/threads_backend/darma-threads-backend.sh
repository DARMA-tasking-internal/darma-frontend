#!/bin/bash

# This script is a template for installing the Darma threads backend.
# Copy this to your build directory, fill in ALL variables in the
# section below, and execute.

####################

# TODO: Path of darma source code relative to build dir
DARMA_SOURCE_DIR=

# TODO: Absolute path to previous frontend install
DARMA_FRONTEND_PREFIX=

# TODO: Path for backend installation relative to build dir
DARMA_BACKEND_PREFIX=

# TODO: C++ compiler path (uncomment and set if not already in your environment)
#CXX=

# TODO: C compiler path (uncomment and set if not already in your environment)
#CC=

# TODO: Cmake build type, i.e., Debug, Release, RelWithDebInfo, or MinSizeRel
DARMA_BUILD_TYPE=Debug

####################

cmake ${DARMA_SOURCE_DIR}/src/reference_backends \
  -DCMAKE_BUILD_TYPE=${DARMA_BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=${DARMA_BACKEND_PREFIX} \
  -DDARMA_FRONTEND_DIR=${DARMA_FRONTEND_PREFIX} \
  -DTHREADS_BACKEND=ON \
  -DCMAKE_CXX_COMPILER=${CXX} \
  -DCMAKE_C_COMPILER=${CC}

if [ $? -eq 0 ]; then
  echo "Type make install to build and install Darma threads backend."
fi

