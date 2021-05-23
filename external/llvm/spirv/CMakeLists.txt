#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# SPIRV source build with prebuilt LLVM.

cmake_minimum_required(VERSION 3.13.4)

project(IGC_SPIRVTranslator)

# Set up LLVM project dependency for translator build.
find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION} REQUIRED)

message(STATUS "[SPIRV] Found LLVM: ${LLVM_DIR}")

set(CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR} ${CMAKE_MODULE_PATH})

include(AddLLVM)
include(HandleLLVMOptions)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

build_llvm_project(SPIRV ${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/build)
