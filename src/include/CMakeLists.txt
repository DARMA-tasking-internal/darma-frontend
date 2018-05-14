
install (FILES darma.h DESTINATION include)

add_subdirectory(darma)

#------------------------------------------------------------------------------#
# tinympl interface library                                                    #
#------------------------------------------------------------------------------#

add_library(tinympl INTERFACE)
add_library(tinympl::tinympl ALIAS tinympl)
add_library(darma::tinympl ALIAS tinympl)

target_include_directories(tinympl INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(tinympl INTERFACE cxx_std_14)

install(TARGETS tinympl EXPORT tinymplTargets
  INCLUDES DESTINATION include
)

install(EXPORT tinymplTargets
  FILE tinymplTargets.cmake
  NAMESPACE tinympl::
  DESTINATION cmake
)

export(TARGETS tinympl
  NAMESPACE tinympl::
  FILE tinymplTargets.cmake
)

install(DIRECTORY tinympl DESTINATION include FILES_MATCHING PATTERN "*.hpp")

