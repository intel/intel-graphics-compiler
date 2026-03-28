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
# Documentation Generation
#=============================================================================

set(DOC_FILENAME "supported-spirv-extensions")


if(NOT DEFINED OUT_DOC_FILE)
    set(OUT_DOC_FILE ${CMAKE_SOURCE_DIR}/documentation/igc/${DOC_FILENAME}.md)
endif()

set(LLVM_TARGET_DEFINITIONS SPIRVExtensions.td)
set(IGCC_LIB_INTERFACES_DOCS_TABLEGEN_EXE "${IGCC_LIB_INTERFACES_TABLEGEN_EXE}")
tablegen(IGCC_LIB_INTERFACES_DOCS ${DOC_FILENAME}.md -gen-igcc-spirv-extensions-docs)
add_public_tablegen_target(IGCCSPIRVExtensionsDocs)

add_custom_command(
    OUTPUT ${OUT_DOC_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/${DOC_FILENAME}.md
            ${OUT_DOC_FILE}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${DOC_FILENAME}.md)

add_custom_target(${DOC_FILENAME}DocGen DEPENDS ${OUT_DOC_FILE})
add_dependencies(${DOC_FILENAME}DocGen IGCCSPIRVExtensionsDocs)


# Create combined docs target
add_custom_target(SPIRVExtensionsDocsGen)
add_dependencies(SPIRVExtensionsDocsGen ${DOC_FILENAME}DocGen)


#=============================================================================
# CPP Headers Generation
#=============================================================================

# Generate C++ header for SPIR-V extensions
set(LLVM_TARGET_DEFINITIONS ${SPIRV_TD_INPUT_FILE})
set(IGCC_LIB_INTERFACES_CPP_TABLEGEN_EXE "${IGCC_LIB_INTERFACES_TABLEGEN_EXE}")
tablegen(IGCC_LIB_INTERFACES_CPP ${CPP_HEADER_FILENAME}.h -gen-igcc-spirv-extension-support-header)
add_public_tablegen_target(IGCCSPIRVExtensionsCpp)

add_custom_target(${CPP_HEADER_FILENAME}Gen
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CPP_HEADER_FILENAME}.h)
add_dependencies(${CPP_HEADER_FILENAME}Gen IGCCSPIRVExtensionsCpp)

# Make C++ generation depend on documentation generation
# This ensures docs are always built when C++ headers are built
add_dependencies(${CPP_HEADER_FILENAME}Gen SPIRVExtensionsDocsGen)
