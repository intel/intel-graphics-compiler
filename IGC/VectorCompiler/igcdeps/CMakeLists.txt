#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Convenience function to get absolute paths for list of files. Mostly
# needed because of different behavior of target_sources between cmake
# versions.
function(vc_get_absolute_paths RET BASE_PATH)
  set(FILES ${ARGN})
  foreach(FILE ${FILES})
    get_filename_component(ABS_FILE ${FILE} ABSOLUTE ${BASE_PATH})
    list(APPEND ABS_FILES ${ABS_FILE})
  endforeach()
  set(${RET} ${ABS_FILES} PARENT_SCOPE)
endfunction()

add_library(VCIGCDeps STATIC "")

target_link_libraries(VCIGCDeps
  VCHeaders

  VCEmbeddedBiF
  VCDriver
  )

add_dependencies(VCIGCDeps
  "${IGC_BUILD__PROJ__GenISAIntrinsics}"
  )

target_include_directories(VCIGCDeps
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

add_subdirectory(src)
