#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Generate the docs for the supported SPIR-V extensions
set(DOC_FILENAME  "supported-spirv-extensions")
set(OUT_DOC_FILE ${CMAKE_SOURCE_DIR}/IGC/AdaptorOCL/docs/autogen/${DOC_FILENAME}.md)

set(LLVM_TARGET_DEFINITIONS SPIRVExtensions.td)
set(IGCC_LIB_INTERFACES_DOCS_TABLEGEN_EXE igcc-yaml-docs-tblgen)
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

# Generate the header with YAML descriptions of the supported SPIR-V extensions
set(IGCC_LIB_INTERFACES_TABLEGEN_EXE igcc-yaml-docs-tblgen)
set(LLVM_TARGET_DEFINITIONS SPIRVExtensions.td)
tablegen(IGCC_LIB_INTERFACES IGCCSPIRVExtensionsYaml.inc -gen-igcc-spirv-extensions-yaml)
add_public_tablegen_target(IGCCSPIRVExtensionsYaml)
add_dependencies(IGCCSPIRVExtensionsYaml IGCCSPIRVExtensionsDocs)
add_dependencies(IGCCSPIRVExtensionsYaml ${DOC_FILENAME}DocGen)
