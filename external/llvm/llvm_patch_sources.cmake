#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# This code is responsible for applying patches to LLVM of specified version.

include_guard(DIRECTORY)

set(IGC_LLVM_SOURCE_DIR ${IGC_LLVM_WORKSPACE_SRC}/llvm)

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
message(STATUS "[LLVM_PATCHER] : Copying stock LLVM sources ${IGC_OPTION__LLVM_SOURCES_DIR} to ${IGC_LLVM_SOURCE_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/.git ${IGC_LLVM_WORKSPACE_SRC}/.git)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/llvm ${IGC_LLVM_SOURCE_DIR})

message(STATUS "[LLVM_PATCHER] : Applying patches for LLVM from version ${DIR_WITH_PATCHES}")

file(GLOB LLVM_PATCH_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/releases/${DIR_WITH_PATCHES}/patches_external/*.patch"
  )
list(SORT LLVM_PATCH_FILES)
# Apply customization patches if any.
foreach(patch_file ${LLVM_PATCH_FILES})
  message(STATUS "[LLVM_PATCHER] : Apply ${patch_file} file")
  execute_process(COMMAND ${Patch_EXECUTABLE} -d ${IGC_LLVM_WORKSPACE_SRC} -p1 -i ${patch_file}
  RESULT_VARIABLE rv
  OUTPUT_VARIABLE ov
  ERROR_VARIABLE ev)
  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "[LLVM_PATCHER] : error: applying patch '${patch_file}' failed.\nstdout:\n${ov}\nstderr:${ev}"
      "Probably you set incorrent version of LLVM (use IGC_OPTION__LLVM_PREFERRED_VERSION option),\n"
      "or passed incorrect sources with different version (use IGC_OPTION__LLVM_SOURCES_DIR option)."
      )
  endif()
endforeach()
