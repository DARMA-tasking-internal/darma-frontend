
export CXX=/opt/local/bin/g++-mp-5
export CC=/opt/local/bin/gcc-mp-5

DARMA_SRC=/Users/fnrizzi/Desktop/projects/atdm/darma
DARMA_BLD=/Users/fnrizzi/Desktop/projects/atdm/darma_build

fe_src=${DARMA_SRC}
fe_pfx=${DARMA_BLD}/fend_install
be_src=${DARMA_SRC}/src/reference_backends
be_pfx=${DARMA_BLD}/be_th_install

SRCPATH=${DARMA_SRC}
buildPfx=${DARMA_BLD}/darma_ex

###################
#install front end
###################
rm -rf ${fe_pfx}/* && mkdir build_fe && cd build_fe
cmake ${fe_src}/src/include -DCMAKE_INSTALL_PREFIX=${fe_pfx}
make install
cd .. && rm -rf build_fe

###########################
#install THREADS back end
##########################
rm -rf ${be_pfx}/* && mkdir build_be && cd build_be
cmake \
	-DCMAKE_CXX_COMPILER:FILEPATH=${CXX} \
	-DCMAKE_C_COMPILER:FILEPATH=${CC}\
	-DCMAKE_BUILD_TYPE="Debug" \
	-DCMAKE_INSTALL_PREFIX=${be_pfx} \
	-DCMAKE_CXX_FLAGS="-std=c++1y" \
	-DDARMA_FRONTEND_DIR=${fe_pfx} \
    -DTHREADS_BACKEND=On \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=on \
	${be_src}
make -j 8
make install 
cd .. && rm -rf build_be

###########################
# examples
##########################
rm -rf ${buildPfx} && mkdir ${buildPfx} && cd ${buildPfx}
cmake \
	-D CMAKE_CXX_COMPILER:FILEPATH=${CXX} \
	-D CMAKE_C_COMPILER:FILEPATH=${CC} \
  	-D DARMA_FRONTEND_DIR=${fe_pfx} \
  	-D THREADS_BACKEND=On \
	-D CMAKE_BUILD_TYPE="Debug" \
 	-D DARMA_BACKEND_DIR=${be_pfx} \
  	-D DARMA_BACKEND_LIBNAME=libdarma_threads_backend.dylib \
	-D CMAKE_VERBOSE_MAKEFILE:BOOL=on \
  	${SRCPATH}
make -j 6
cd ..
