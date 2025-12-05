#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(${CMAKE_CURRENT_LIST_DIR}/BiFMCConst.cmake)

file(READ ${BiFModule_SRC_SHA_PATH} BiF_SRC_CHECKSUM)
if(EXISTS ${BiFModule_PreBuild_PATH})
    file(REMOVE_RECURSE ${BiFModule_PreBuild_PATH})
endif()
file(MAKE_DIRECTORY ${BiFModule_PreBuild_PATH})
string(REPLACE " " ";" bifDeps ${LIST_FILES})
foreach(_bifDep ${bifDeps})
    if(${_bifDep} MATCHES ".*\\.[bch]+$")
        message("[IGC\\BiFModuleCache] - Copying file ${_bifDep} to prebuild pack")
        file(COPY "${_bifDep}" DESTINATION "${BiFModule_PreBuild_PATH}/")
    endif()
endforeach()
file(WRITE "${BiFModule_PREBUILD_SHA_PATH}" ${BiF_SRC_CHECKSUM})
message("[IGC\\BiFModuleCache] - Prebuild pack with checksum [${BiF_SRC_CHECKSUM}] is ready")


