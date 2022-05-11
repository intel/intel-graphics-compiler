#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

string(REPLACE "$(Configuration)" "" IGC_BUILD__BIF_ROOT_DIR ${IGC_BUILD__BIF_DIR})
message("[IGC\\BiFModuleCache] - IGC_BUILD__BIF_ROOT_DIR: ${IGC_BUILD__BIF_ROOT_DIR}")

include(BiFMCConst)
include(BiFMCGetListFiles)

set(BiFModule_SRC_LIST "")

get_bif_src_list(${opencl-header} ${BiFModule_SRC} BiFModule_SRC_LIST)

message("[IGC\\BiFModuleCache] - BiFModule_SRC: ${BiFModule_SRC}")

if(NOT EXISTS ${IGC_BUILD__BIF_ROOT_DIR})
    file(MAKE_DIRECTORY ${IGC_BUILD__BIF_ROOT_DIR})
endif()

file(REMOVE ${BiFModule_SRC_SHA_PATH})
file(REMOVE ${BiFModule_Init})

add_custom_command(
    OUTPUT ${BiFModule_Init}
    COMMAND ${CMAKE_COMMAND}
        -Dopencl-header=${opencl-header}
        -DIGC_BUILD__BIF_ROOT_DIR=${IGC_BUILD__BIF_ROOT_DIR}
        -DIGC_BUILD__BIF_DIR=${IGC_BUILD__BIF_DIR}
        -DBiFModuleCacheTarget=$<IF:$<CONFIG:Release>,"Release","Non-Release">
        -P ${CMAKE_CURRENT_LIST_DIR}/BiFMCInit.cmake
    COMMENT "Running BiFMCInit to prepare BiFModule caching"
    DEPENDS "${BiFModule_SRC_SHA_PATH}"
)

set(IGC_BUILD__PROJ__BiFModuleCache_INIT       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCache_INIT")
set(IGC_BUILD__PROJ__BiFModuleCache_INIT       "${IGC_BUILD__PROJ__BiFModuleCache_INIT}" PARENT_SCOPE)
add_custom_target("${IGC_BUILD__PROJ__BiFModuleCache_INIT}"
    DEPENDS "${BiFModule_Init}"
)

# Function generates BiFModule-SRC checksum
# @param bifModuleTgt BiFModule target
function(generate_bif_src_checksum bifModuleTgt)
    add_custom_command(
        OUTPUT ${BiFModule_SRC_SHA_PATH}
        COMMAND ${CMAKE_COMMAND}
            -Dopencl-header=${opencl-header}
            -DIGC_BUILD__BIF_ROOT_DIR=${IGC_BUILD__BIF_ROOT_DIR}
            -DBiFModuleCacheTarget=$<IF:$<CONFIG:Release>,"Release","Non-Release">
            -P ${CMAKE_CURRENT_LIST_DIR}/cmake/BiFMCChecksum.cmake
        COMMENT "Running BiFMCChecksum to update checksum for BiFModule sources"
        DEPENDS ${BiFModule_SRC_LIST}
    )

    set(IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCache_SRC_checksum")
    set(IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL       "${IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL}" PARENT_SCOPE)
    add_custom_target("${IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL}"
        DEPENDS "${BiFModule_SRC_SHA_PATH}"
    )

add_dependencies(${bifModuleTgt} ${IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL})

endfunction()


# Function generates BiFModule-Prebuild pack
# @param bifModuleDepends BiFModule depends
# @param bifModuleTgt BiFModule target
function(generate_bif_prebuild_pack bifModuleTgt bifModuleDepends)
    add_custom_command(
        OUTPUT ${BiFModule_PREBUILD_SHA_PATH}
        COMMAND ${CMAKE_COMMAND}
            -DLIST_FILES="${bifModuleDepends}"
            -DIGC_BUILD__BIF_ROOT_DIR=${IGC_BUILD__BIF_ROOT_DIR}
            -DBiFModuleCacheTarget=$<IF:$<CONFIG:Release>,"Release","Non-Release">
            -P ${CMAKE_CURRENT_LIST_DIR}/cmake/BiFMCBuild.cmake
        COMMENT "Running BiFMCBuild to create BiFModule cache package"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS "${BiFModule_SRC_SHA_PATH}"
    )

    set(IGC_BUILD__PROJ__BiFModuleCache_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCache")
    set(IGC_BUILD__PROJ__BiFModuleCache_OCL       "${IGC_BUILD__PROJ__BiFModuleCache_OCL}" PARENT_SCOPE)
    add_custom_target("${IGC_BUILD__PROJ__BiFModuleCache_OCL}"
        DEPENDS ${bifModuleTgt} "${BiFModule_PREBUILD_SHA_PATH}"
    )
endfunction()
