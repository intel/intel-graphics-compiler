#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2020-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
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

  VCCodeGen
  )

add_dependencies(VCIGCDeps
  "${IGC_BUILD__PROJ__GenISAIntrinsics}"
  )

target_include_directories(VCIGCDeps
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

add_subdirectory(src)
