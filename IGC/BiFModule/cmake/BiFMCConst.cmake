#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Needs defined var IGC_BUILD__BIF_ROOT_DIR

set(BiFModule_CHECKSUM_FILE "BiFModule_PREB.sha")
set(BiFModule_SRC "${CMAKE_CURRENT_LIST_DIR}/../")
set(BiFModule_PreBuild_PATH "${IGC_BUILD__BIF_ROOT_DIR}prebuild")
set(BiFModule_SRC_SHA_PATH "${IGC_BUILD__BIF_ROOT_DIR}BiFModule_SRC.sha")
set(BiFModule_PREBUILD_SHA_PATH "${BiFModule_PreBuild_PATH}/${BiFModule_CHECKSUM_FILE}")
set(BiFModule_Init ${IGC_BUILD__BIF_ROOT_DIR}bifModuleInit)

set(BiFModule_PreBuild_FileList
    BiFModule_PREB.sha
    OCLBiFImpl.h
    opencl_cth.h
    OCLBiFImpl.bifbc
    IBiF_Impl_int_spirv.bc
    OCLBiFImpl.bc
    IGCsize_t_32.bc
    IGCsize_t_64.bc)