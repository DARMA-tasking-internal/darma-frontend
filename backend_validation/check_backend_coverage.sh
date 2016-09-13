#!/bin/bash

# requires gcov and perl
# make sure to build with -fprofile-arcs -ftest-coverage -g -O0

if [ -z $1 ]; then
  echo "Specify object file on the command line."
  exit 1
fi

# name of object file without path
OBJFILE=$(basename $1)

# store current absolute path, since other paths used might be relative
INVOCATION_DIR=$(pwd)

# now get this script's absolute path
cd $(dirname $0)
BACKEND_VALIDATION_ROOT=$(pwd)

# go back to where we started in case the object file had relative path
cd ${INVOCATION_DIR}

# now go to the object file's path
cd $(dirname $1)

# finally, check coverage
gcov -f -n ${OBJFILE} | perl ${BACKEND_VALIDATION_ROOT}/print_backend_coverage.pl ${BACKEND_VALIDATION_ROOT}/backend_functions.txt

