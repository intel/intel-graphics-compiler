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


# Function generates BiFModule-SRC checksum
# @param bifModuleTgt BiFModule target
function(generate_bif_src_checksum bifModuleTgt)
    set(BIF_ARCH "${IGC_OPTION__ARCHITECTURE_TARGET}")
    add_custom_command(
        OUTPUT ${BiFModule_SRC_SHA_PATH}
        COMMAND ${CMAKE_COMMAND}
            -Dopencl-header=${opencl-header}
            -DIGC_BUILD__BIF_ROOT_DIR=${IGC_BUILD__BIF_ROOT_DIR}
            -DBIF_ARCH="${BIF_ARCH}"
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
    set_target_properties("${IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL}" PROPERTIES FOLDER "Misc/BiF")

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

    set(IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCacheBuildPack")
    set(IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL       "${IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL}" PARENT_SCOPE)
    add_custom_target("${IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL}"
        DEPENDS ${bifModuleTgt} "${BiFModule_PREBUILD_SHA_PATH}"
    )
    set_target_properties("${IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL}" PROPERTIES FOLDER "Misc/BiF")
endfunction()


# Function generates BiFModule-Prebuild pack
# @param bifModuleDepends BiFModule depends
function(build_bif_bitcode bifModuleDepends)
  set(bif-llvm-as_exe ${LLVM_AS_EXE})
  set(bif-llvm-dis_exe ${LLVM_DIS_EXE})
  set(bif-llvm-link_exe ${LLVM_LINK_EXE})
  set(bif-llvm-opt_exe ${LLVM_OPT_EXE})

  if(NOT (IGC_OPTION__LLVM_MODE STREQUAL PREBUILDS_MODE_NAME AND WIN32))
    if(NOT EXISTS ${LLVM_AS_EXE})
      set(bif-llvm-as_exe $<TARGET_FILE:${LLVM_AS_EXE}>)
    endif()
    if(NOT EXISTS ${LLVM_DIS_EXE} AND TARGET ${LLVM_DIS_EXE})
      set(bif-llvm-dis_exe $<TARGET_FILE:${LLVM_DIS_EXE}>)
    endif()
    if(NOT EXISTS ${LLVM_LINK_EXE})
      set(bif-llvm-link_exe $<TARGET_FILE:${LLVM_LINK_EXE}>)
    endif()
    if(NOT EXISTS ${LLVM_OPT_EXE})
      set(bif-llvm-opt_exe $<TARGET_FILE:${LLVM_OPT_EXE}>)
    endif()
  endif()

    add_custom_command(
        OUTPUT ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.h ${IGC_BUILD__BIF_DIR}/opencl_cth.h ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bifbc ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc ${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc ${IGC_BUILD__BIF_DIR}/IBiF_BFloat.bc ${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc ${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc
        COMMAND ${CMAKE_COMMAND}
            -DIGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG=${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG}
            -DIGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT=${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT}
            -Dbif-llvm-as_exe=${bif-llvm-as_exe}
            -Dbif-llvm-dis_exe=${bif-llvm-dis_exe}
            -Dbif-llvm-link_exe=${bif-llvm-link_exe}
            -Dbif-llvm-opt_exe=${bif-llvm-opt_exe}
            -DIGC_BUILD__BIF_OCL_SHARED_INC="${IGC_BUILD__BIF_OCL_SHARED_INC}"
            -DIGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE="${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}"
            -DIGC_OPTION__BIF_SRC_OCL_DIR="${IGC_OPTION__BIF_SRC_OCL_DIR}"
            -DIGC_OPTION__ENABLE_BF16_BIF="${IGC_OPTION__ENABLE_BF16_BIF}"
            -DIGC_OPTION__BIF_UPDATE_IR="${IGC_OPTION__BIF_UPDATE_IR}"
            -DVME_TYPES_DEFINED="${VME_TYPES_DEFINED}"
            -D_PRE_RELEASE_CL="${_PRE_RELEASE_CL}"
            -DIGC_SOURCE_DIR="${IGC_SOURCE_DIR}"
            -DIGC_BUILD__BIF_DIR="${IGC_BUILD__BIF_DIR}"
            -DCL_OPTIONS="${CL_OPTIONS}"
            -DIGC_BUILD__BIF_OCL_INCLUDES="${IGC_BUILD__BIF_OCL_INCLUDES}"
            -DPYTHON_EXECUTABLE="${PYTHON_EXECUTABLE}"
            -Dclang-tool=$<TARGET_FILE:clang-tool>
            -DBiFManager-bin=$<TARGET_FILE:BiFManager-bin>
            -DIGC_BUILD__BIF_ROOT_DIR="${IGC_BUILD__BIF_ROOT_DIR}"
            -DVME_TYPES_DEFINED="${VME_TYPES_DEFINED}"
            -DBIF_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            -DLLVM_VERSION_MAJOR="${LLVM_VERSION_MAJOR}"
            -DIGC_BUILD__BIF_DIR="${IGC_BUILD__BIF_DIR}"
            -DBiFModuleCacheTarget=$<IF:$<CONFIG:Release>,"Release","Non-Release">
            -P ${CMAKE_CURRENT_LIST_DIR}/cmake/BiFBuildBitcode.cmake
        COMMENT "Running BiFBuildBitcode script"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS ${BiFModule_SRC_SHA_PATH} $<TARGET_FILE:BiFManager-bin>
        DEPENDS "${IGC_BUILD__PROJ__IBiF_matrix_generator}"
        COMMAND_EXPAND_LISTS
    )

    set(IGC_BUILD__PROJ__BiFModuleCache_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCache")
    set(IGC_BUILD__PROJ__BiFModuleCache_OCL       "${IGC_BUILD__PROJ__BiFModuleCache_OCL}" PARENT_SCOPE)
    add_custom_target("${IGC_BUILD__PROJ__BiFModuleCache_OCL}"
        DEPENDS ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.h ${IGC_BUILD__BIF_DIR}/opencl_cth.h ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bifbc ${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc ${IGC_BUILD__BIF_DIR}/IBiF_BFloat.bc ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc ${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc ${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc
    )
    set_target_properties("${IGC_BUILD__PROJ__BiFModuleCache_OCL}" PROPERTIES FOLDER "Misc/BiF")
endfunction()
