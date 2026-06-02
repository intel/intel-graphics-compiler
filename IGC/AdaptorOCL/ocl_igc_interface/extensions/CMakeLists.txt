#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGCC_LIB_INTERFACES_TABLEGEN_EXE igcc-spirv-support-tblgen)

# Optional: native host igcc-spirv-support-tblgen when provided by the build system
set(IGC_NATIVE_IGCC_SPIRV_SUPPORT_TBLGEN "" CACHE FILEPATH
    "Path to native igcc-spirv-support-tblgen executable (if set, overrides the in-tree target)")

if(IGC_NATIVE_IGCC_SPIRV_SUPPORT_TBLGEN)
    message(STATUS "Overriding igcc-spirv-support-tblgen with IGC_NATIVE_IGCC_SPIRV_SUPPORT_TBLGEN=${IGC_NATIVE_IGCC_SPIRV_SUPPORT_TBLGEN}")
    set(IGCC_LIB_INTERFACES_TABLEGEN_EXE "${IGC_NATIVE_IGCC_SPIRV_SUPPORT_TBLGEN}")
endif()

# Default configuration for open source
set(SPIRV_TD_INPUT_FILE "SPIRVExtensions.td")
set(CPP_HEADER_FILENAME "SPIRVExtensionsSupport")



#=============================================================================
# CPP Headers Generation
#=============================================================================

# Generate data .inc file for SPIR-V extensions (included by the hand-written
# SPIRVExtensionsSupport.h header in this source directory).
set(TABLEGEN_OUTPUT)
set(LLVM_TARGET_DEFINITIONS ${SPIRV_TD_INPUT_FILE})
set(IGCC_LIB_INTERFACES_CPP_TABLEGEN_EXE "${IGCC_LIB_INTERFACES_TABLEGEN_EXE}")
tablegen(IGCC_LIB_INTERFACES_CPP ${CPP_HEADER_FILENAME}.inc
         -gen-igcc-spirv-extension-support-header)
add_public_tablegen_target(IGCCSPIRVExtensionsCpp)

add_custom_target(${CPP_HEADER_FILENAME}Gen
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CPP_HEADER_FILENAME}.inc)
add_dependencies(${CPP_HEADER_FILENAME}Gen IGCCSPIRVExtensionsCpp)

# The hand-written .h lives in the source tree; the .inc it includes is in the
# binary tree.  Both directories must be on the include path for consumers.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

