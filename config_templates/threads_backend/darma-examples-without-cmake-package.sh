#!/bin/bash

# This script is a template for building the Darma examples against
# the threads backend.  Copy this to your build directory, fill in ALL
# variables in the section below, and execute.

####################

# TODO: Path of darma source code relative to build dir
DARMA_SOURCE_DIR=

# TODO: Absolute path to previous frontend install
DARMA_FRONTEND_PREFIX=

# TODO: Absolute path to previous backend install
DARMA_BACKEND_PREFIX=

# TODO: C++ compiler path (uncomment and set if not already in your environment)
#CXX=

# TODO: C compiler path (uncomment and set if not already in your environment)
#CC=

# TODO: pthread library
PTHREAD_LIB=-lpthread

# TODO: Cmake build type, i.e., Debug, Release, RelWithDebInfo, or MinSizeRel
DARMA_BUILD_TYPE=Debug

####################

cmake ${DARMA_SOURCE_DIR}/examples \
  -DCMAKE_BUILD_TYPE=${DARMA_BUILD_TYPE} \
  -DDARMA_FRONTEND_DIR=${DARMA_FRONTEND_PREFIX} \
  -DDARMA_BACKEND_DIR=${DARMA_BACKEND_PREFIX} \
  -DDARMA_BACKEND_LIBNAME=darma_threads_backend \
  -DCMAKE_CXX_COMPILER=${CXX} \
  -DCMAKE_C_COMPILER=${CC} \
  -DCMAKE_EXE_LINKER_FLAGS="${PTHREAD_LIB}"

if [ $? -eq 0 ]; then
  echo "Type make build the darma examples against the installed threads backend."
fi

