#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include("${IGC_BIF_CONFIG}")
include(${CMAKE_CURRENT_LIST_DIR}/BiFMCConst.cmake)

file(READ ${BiFModule_SRC_SHA_PATH} BiF_SRC_CHECKSUM)
if(EXISTS ${BiFModule_PreBuild_PATH})
    file(REMOVE_RECURSE ${BiFModule_PreBuild_PATH})
endif()
file(MAKE_DIRECTORY ${BiFModule_PreBuild_PATH})
foreach(output IN LISTS BiFModule_PreBuild_FileList)
    if(output STREQUAL "${BiFModule_CHECKSUM_FILE}")
      continue()
    endif()
    file(COPY "${IGC_BUILD__BIF_DIR}/${output}" DESTINATION "${BiFModule_PreBuild_PATH}/")
endforeach()
file(WRITE "${BiFModule_PREBUILD_SHA_PATH}" ${BiF_SRC_CHECKSUM})
message("[IGC\\BiFModuleCache] - Prebuild pack with checksum [${BiF_SRC_CHECKSUM}] is ready")


