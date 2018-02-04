# Compiler-specific C++14 activation.
# Call this from all CMakeLists.txt files that can be built independently.

macro(set_darma_compiler_flags)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # 4.9.3 complains about std::min not being constexpr
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
  execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if (NOT (GCC_VERSION VERSION_GREATER 5 OR GCC_VERSION VERSION_EQUAL 5))
    message("${PROJECT_NAME} currently requires g++ 5 or greater.  If you need it to work with 4.9, please complain.")
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -ftemplate-depth=900")
  if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # 16.0.3 complains about std::min not being constexpr
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
else ()
  message(FATAL_ERROR "Your C++ compiler may not support C++14.")
endif ()

endmacro()


# Function that checks for presence of python module
# Found at 

function(find_python_module module)
  string(TOUPPER ${module} module_upper)
  if(NOT PY_${module_upper})
    if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
      set(${module}_FIND_REQUIRED TRUE)
    endif()

# A module's location is usually a directory, but for binary modules
# it's a .so file.
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c" 
      "import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
      RESULT_VARIABLE _${module}_status 
      OUTPUT_VARIABLE _${module}_location
      ERROR_QUIET 
      OUTPUT_STRIP_TRAILING_WHITESPACE)
		
    if(NOT _${module}_status)
      set(PY_${module_upper} ${_${module}_location} CACHE STRING 
      "Location of Python module ${module}")
    endif(NOT _${module}_status)
  endif(NOT PY_${module_upper})
  find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)
