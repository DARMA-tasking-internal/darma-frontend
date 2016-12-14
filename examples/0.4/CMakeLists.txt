set (EXAMPLES_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set (EXAMPLES_BIN_ROOT ${CMAKE_CURRENT_BINARY_DIR})

if (DARMA_ALL_IN_ONE)
	set (DARMA_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/src)
	#figure out which backend
	include_directories( ${CMAKE_SOURCE_DIR}/src/reference_backends )
	if (SERIAL_BACKEND)  
	set (backend_name serial)
	elseif (THREADS_BACKEND)
	set (backend_name threads)
	find_package ( Threads REQUIRED )
	endif()
	set(backend_dir ${CMAKE_SOURCE_DIR}/src/reference_backends/${backend_name})
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${backend_dir}")
	set(DARMA_BACKEND_LIBNAME darma_${backend_name}_backend ${CMAKE_THREAD_LIBS_INIT})
	include_directories(${backend_dir})
	include_directories( ${DARMA_FRONTEND_DIR}/include )
	include_directories( ${DARMA_FRONTEND_DIR}/include/tinympl )
	include_directories( ${DARMA_FRONTEND_DIR}/include/darma )

else() #built as standalone

	if (DARMA_BACKEND_PKG)
		find_package (${DARMA_BACKEND_PKG})
		if (NOT ${DARMA_BACKEND_PKG}_FOUND)
		  message (FATAL_ERROR "Error: Darma backend package could not be found")
		endiF ()

		set (CMAKE_CXX_COMPILER ${DARMA_BACKEND_CXX_COMPILER})
		if (NOT CMAKE_CXX_FLAGS)
		  set (CMAKE_CXX_FLAGS ${DARMA_BACKEND_CXX_FLAGS})
		endif ()

		include_directories( ${DARMA_BACKEND_INCLUDE_DIRS} )
		link_directories( ${DARMA_BACKEND_LIBRARY_DIRS} )
		# DARMA_BACKEND_LIBNAME might contain multiple libraries
		set( DARMA_BACKEND_LIBNAME ${DARMA_BACKEND_LIBRARIES} )

	else()  # not using cmake package

		if (NOT DARMA_BACKEND_LIBNAME)
		message(FATAL_ERROR
		    "Must define backend package via -DDARMA_BACKEND_PKG= or give library name for backend (e.g. for libdarma_backend_test.a give -DDARMA_BACKEND_LIBNAME=darma_backend_test)")
		endif()

		set (DARMA_BACKEND_LIBNAME ${DARMA_BACKEND_LIBNAME} ${CMAKE_THREAD_LIBS_INIT})

		if (NOT DARMA_FRONTEND_DIR)
		message(FATAL_ERROR "Must define prefix install dir for frontend via -DDARMA_FRONTEND_DIR=")
		endif()

		if (NOT DARMA_BACKEND_DIR)
		message(FATAL_ERROR "Must define prefix install dir for backend via -DDARMA_BACKEND_DIR=")
		endif()

		link_directories( ${DARMA_BACKEND_DIR}/lib )

		include_directories( ${DARMA_BACKEND_DIR}/include )
		include_directories( ${DARMA_FRONTEND_DIR}/include )
		include_directories( ${DARMA_FRONTEND_DIR}/include/tinympl )
		include_directories( ${DARMA_FRONTEND_DIR}/include/darma )
	endif()

endif() #DARMA_ALL_IN_ONE

include_directories( ${EXAMPLES_SRC_ROOT} )
include_directories( ${EXAMPLES_BIN_ROOT} )

include (../../cmake-modules/SetDarmaCompilerFlags.cmake)
set_darma_compiler_flags()

# all extra arguments to add_test_for_example are used as arguments for
# executing the test itself
macro(add_test_for_example test_name test_exec)
if (NOT CTEST_SKIP_EXAMPLES)
add_test(${test_name} ${test_exec} ${ARGN})
set_tests_properties(${test_name} PROPERTIES TIMEOUT 60
  FAIL_REGULAR_EXPRESSION "FAILED;WARNING")
endif()
endmacro()

#add_subdirectory(fib)
#add_subdirectory(fib_region)
#add_subdirectory(fib_nokey)
#add_subdirectory(region_square_roots)
#add_subdirectory(persistent_square_roots)
#add_subdirectory(simple_reduce)
add_subdirectory(simple_collection)
add_subdirectory(simple_collection_iter)
add_subdirectory(simple_collection_fetch)
#add_subdirectory(simple_collection_unique_owner)

