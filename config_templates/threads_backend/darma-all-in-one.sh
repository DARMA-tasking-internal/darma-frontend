#!/bin/bash

# This script is a template for an all-in-one (frontend, threads
# backend, and examples) install of Darma.  Copy this to your build
# directory, fill in ALL variables in the section below, and execute.

####################

# TODO: Path of darma source code relative to build dir
DARMA_SOURCE_DIR=

# TODO: Path for installation relative to build dir
DARMA_INSTALL_PREFIX=

# TODO: C++ compiler path (uncomment and set if not already in your environment)
#CXX=

# TODO: C compiler path (uncomment and set if not already in your environment)
#CC=

# TODO: Cmake build type, i.e., Debug, Release, RelWithDebInfo, or MinSizeRel
DARMA_BUILD_TYPE=Debug

####################

cmake ${DARMA_SOURCE_DIR} \
  -DCMAKE_BUILD_TYPE=${DARMA_BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=${DARMA_INSTALL_PREFIX} \
  -DTHREADS_BACKEND=ON \
  -DCMAKE_CXX_COMPILER=${CXX} \
  -DCMAKE_C_COMPILER=${CC}

if [ $? -eq 0 ]; then
  echo "Type make install to build and install Darma all-in-one with the threads backend."
fi

