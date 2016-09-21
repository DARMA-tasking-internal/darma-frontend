#!/bin/bash

# This script is a template for building the Darma examples against
# the threads backend.  It will automatically use the same compilers
# and flags used to build the backend, so they should not be set again
# here.  It will build using the same frontend headers used to build
# the backend.  Copy this to your build directory, fill in ALL
# variables in the section below, and execute.

####################

# TODO: Path of darma source code relative to build dir
DARMA_SOURCE_DIR=

# TODO: Absolute path to previous threads backend install
DARMA_BACKEND_PREFIX=

# TODO: Cmake build type, i.e., Debug, Release, RelWithDebInfo, or MinSizeRel
DARMA_BUILD_TYPE=Debug

####################

cmake ${DARMA_SOURCE_DIR}/examples \
  -DCMAKE_BUILD_TYPE=${DARMA_BUILD_TYPE} \
  -DDARMA_BACKEND_PKG=DarmaThreadsBackend \
  -DDarmaThreadsBackend_DIR=${DARMA_BACKEND_PREFIX}/cmake

if [ $? -eq 0 ]; then
  echo "Type make build the darma examples against the installed threads backend."
fi

