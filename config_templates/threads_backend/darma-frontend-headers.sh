#!/bin/bash

# This script is a template for a header-only install of the Darma
# frontend.  Copy this to your build directory, fill in ALL variables
# in the section below, and execute.

####################

# TODO: Path of darma source code relative to build dir
DARMA_SOURCE_DIR=

# TODO: Path for frontend installation relative to build dir
DARMA_FRONTEND_PREFIX=

####################

cmake ${DARMA_SOURCE_DIR}/src/include \
  -DCMAKE_INSTALL_PREFIX=${DARMA_FRONTEND_PREFIX}

if [ $? -eq 0 ]; then
  echo "Type make install to install darma frontend headers."
fi

