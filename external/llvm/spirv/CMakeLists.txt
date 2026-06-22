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

# The translator is part of the compiler, but its targets are created by LLVM's
# add_llvm_library in a separate directory subtree, so they bypass the per-config
# build settings IGC injects for its own targets. IGC carries these as per-config
# generator expressions (NDEBUG and /O2 for non-Debug configs) rather than in the
# shared CMAKE_CXX_FLAGS_<config>, which are identical across configs and contain
# neither -- see Source/IGC/CMakeLists.txt. Without re-applying them here the
# from-source translator compiles with asserts live AND unoptimized in
# Release/ReleaseInternal. Mirror IGC's per-config injection so the translator
# matches the release build; Debug intentionally keeps asserts and stays unoptimized.
add_compile_definitions($<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
if(MSVC)
  add_compile_options($<$<NOT:$<CONFIG:Debug>>:/O2>)
else()
  add_compile_options($<$<NOT:$<CONFIG:Debug>>:-O2>)
endif()

build_llvm_project(SPIRV ${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/build)
