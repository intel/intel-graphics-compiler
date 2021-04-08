#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
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

# This code is responsible for applying patches to LLVM of specified version.

include_guard(DIRECTORY)

if(NOT IGC_LLVM_SOURCE_DIR)
  set(IGC_LLVM_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/../src/llvm")
endif()

# Already copied and patched. Probably...
# TODO: handle dependencies on patches changes.
if(EXISTS "${IGC_LLVM_SOURCE_DIR}")
  return()
endif()

file(GLOB dirsPatch RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/releases ${CMAKE_CURRENT_SOURCE_DIR}/releases/*)
list(SORT dirsPatch)
list(REVERSE dirsPatch)

string(REGEX MATCH "[0-9]+" LLVM_VERSION_MAJOR ${IGC_OPTION__LLVM_PREFERRED_VERSION})

# Customization patches will be applied if present.
foreach(dirPatch ${dirsPatch})
  string(REGEX MATCH "[0-9]+" LLVM_VER_MAJOR_FOLDER ${dirPatch})
  if(${LLVM_VERSION_MAJOR} MATCHES ${LLVM_VER_MAJOR_FOLDER})
    set(DIR_WITH_PATCHES ${dirPatch})
    break()
  endif()
endforeach()

if(NOT DIR_WITH_PATCHES)
  message(STATUS "[LLVM_PATCHER] : No patches found, using stock llvm")
  set(IGC_LLVM_SOURCE_DIR ${IGC_OPTION__LLVM_SOURCES_DIR}/llvm)
  return()
endif()

# Copy stock LLVM sources to IGC_LLVM_SOURCE_DIR to apply patches.
message(STATUS "[LLVM_PATCHER] : Copying stock LLVM and CLANG sources ${LLVM_SOURCE_URL} to ${IGC_LLVM_SOURCE_DIR}/../")
message(STATUS "[LLVM_PATCHER] : Copying stock LLVM and CLANG sources ${IGC_OPTION__LLVM_SOURCES_DIR} to ${IGC_LLVM_SOURCE_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/.git ${IGC_LLVM_SOURCE_DIR}/../.git)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/clang ${IGC_LLVM_SOURCE_DIR}/../clang)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/llvm ${IGC_LLVM_SOURCE_DIR})

message(STATUS "[LLVM_PATCHER] : Applying patches for LLVM from version ${DIR_WITH_PATCHES}")

file(GLOB LLVM_PATCH_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/releases/${DIR_WITH_PATCHES}/patches_external/*.patch"
  )
list(SORT LLVM_PATCH_FILES)
# Apply customization patches if any.
foreach(patch_file ${LLVM_PATCH_FILES})
  message(STATUS "[LLVM_PATCHER] : Apply ${patch_file} file")
  execute_process(COMMAND ${Patch_EXECUTABLE} -d ${IGC_LLVM_SOURCE_DIR} -p1 -i ${patch_file} RESULT_VARIABLE rv)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "[LLVM_PATCHER] : error: applying patch '${patch_file}' failed.\n"
      "Probably you set incorrent version of LLVM (use IGC_OPTION__LLVM_PREFERRED_VERSION option),\n"
      "or passed incorrect sources with different version (use IGC_OPTION__LLVM_SOURCES_DIR option)."
      )
  endif()
endforeach()
