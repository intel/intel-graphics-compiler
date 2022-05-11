#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#Input vars
# @param BiFModule_SRC_LIS - list of all files to by
# @param BiFModule_SRC_SHA_PATH - path to dump the file with checksum for sources

include(${CMAKE_CURRENT_LIST_DIR}/BiFMCConst.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/BiFMCGetListFiles.cmake)

file(REMOVE ${BiFModule_SRC_SHA_PATH})

get_bif_src_list(${opencl-header} ${BiFModule_SRC} BiFModule_SRC_LIST)

# Collect all checksum of all source files into one file
set(allFilesChecksum "")
foreach(_file ${BiFModule_SRC_LIST})
  file(SHA1 ${_file} fileChecksum)
  set(allFilesChecksum "${allFilesChecksum}${fileChecksum}")
  #message("[IGC\\BiFModuleCache] - File : ${_file} SHA1[${fileChecksum}]")
endforeach()

message("[IGC\\BiFModuleCache] - Checksum will be for ${BiFModuleCacheTarget} target")
set(allFilesChecksum "${allFilesChecksum}-${BiFModuleCacheTarget}")

file(WRITE "${BiFModule_SRC_SHA_PATH}.tmp" ${allFilesChecksum})

# Generate the checksum of all sources of BiFModule
file(SHA1 "${BiFModule_SRC_SHA_PATH}.tmp" BiF_SRC_CHECKSUM)

file(REMOVE "${BiFModule_SRC_SHA_PATH}.tmp")

message("[IGC\\BiFModuleCache] - Final source sha1 : ${BiF_SRC_CHECKSUM}")

file(WRITE "${BiFModule_SRC_SHA_PATH}" ${BiF_SRC_CHECKSUM})