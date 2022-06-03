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
set(PATCH_DISABLE "None")

# Already copied and patched. Probably...
# TODO: handle dependencies on patches changes.
if(EXISTS "${IGC_LLVM_SOURCE_DIR}")
  return()
endif()

# Copy stock LLVM sources to IGC_LLVM_SOURCE_DIR to apply patches.
message(STATUS "[LLVM] : Copying stock LLVM sources ${IGC_OPTION__LLVM_SOURCES_DIR} to ${IGC_LLVM_SOURCE_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/.git ${IGC_LLVM_WORKSPACE_SRC}/.git)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/llvm ${IGC_LLVM_SOURCE_DIR})

# In LLVM 14 a /cmake common utils dir was created in the project's root directory
# that's used by all llvm, clang & lld as relative import.
# It may be better to put this is another *.cmake file, however it's convenient here
# as it should always happen when we copy LLVM to out of it's original directory.
if(EXISTS ${IGC_OPTION__LLVM_SOURCES_DIR}/cmake)
  message(STATUS "[LLVM] : Copying LLVM common CMake utils ${IGC_OPTION__LLVM_SOURCES_DIR}/cmake to ${IGC_LLVM_WORKSPACE_SRC}/cmake")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__LLVM_SOURCES_DIR}/cmake ${IGC_LLVM_WORKSPACE_SRC}/cmake)
endif()

message(STATUS "[LLVM] : Applying patches for LLVM from version ${DIR_WITH_PATCHES}")

execute_process(COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/apply_patches.py
  --llvm-version ${IGC_OPTION__LLVM_PREFERRED_VERSION}
  --llvm-project-dir ${IGC_LLVM_WORKSPACE_SRC}
  --patches-dir ${CMAKE_CURRENT_SOURCE_DIR}/releases
  --patch-executable ${Patch_EXECUTABLE}
  --patch-disable ${PATCH_DISABLE}
  RESULT_VARIABLE PATCH_SCRIPT_RESULT
)

if(NOT PATCH_SCRIPT_RESULT EQUAL 0)
  message(FATAL_ERROR "[LLVM] : Could not apply LLVM patches.")
endif()
