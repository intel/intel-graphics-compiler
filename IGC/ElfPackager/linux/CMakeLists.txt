#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# ======================================================================================================
# ================================================ UTILS ===============================================
# ======================================================================================================

# =============================================== Tools ================================================

add_executable(IGC_TARGET__TOOLS_RESOURCE_LINKER IMPORTED)

igc_arch_get_os(_archOs "${IGC_OPTION__ARCHITECTURE_HOST}")

set(IGC_TOOLS_RESOURCE_LINKER  "${IGC_SOURCE_DIR}/BiFModule/resource_embedder.py")

set(IGC_BUILD__BIF_EMBEDDER_DIR "${IGC_BUILD__BIF_DIR}/embedder")

# ========================================= Helper functions ===========================================

# Adds custom step which embeds file into symbol (C++).
#
# @param symbolFilesVarName Name of variable placeholder where path to output symbol file will be appended.
# @param symbolName         Symbol name (must be proper C/C++ identifier and must not end with "_size" suffix).
# @param embeddedFilePath   Path to embedded file (either built or pre-built).
# @param [dependency [...]] Additional dependencies to build targets which build the required files.
#                           If not specified the file is treated as pre-built (and added directly as dependency).
function(igc_resource_embed_file symbolFilesVarName symbolName embeddedFilePath)
  if((NOT (symbolName MATCHES "^[a-zA-Z_][a-zA-Z0-9_]*$")) OR (symbolName MATCHES "_size$"))
    message(FATAL_ERROR "Symbol name \"${symbolName}\" is invalid.")
  endif()

  get_filename_component(_embeddedFileNameWoExt "${embeddedFilePath}" NAME_WE)
  set(_symbolFilePath "${IGC_BUILD__BIF_EMBEDDER_DIR}/${_embeddedFileNameWoExt}_${symbolName}.cpp")

  list(LENGTH ARGN _paramsCount)
  if(_paramsCount GREATER 0)
    set(_dependency)
  else()
    set(_dependency "${embeddedFilePath}")
  endif()

  add_custom_command(OUTPUT "${_symbolFilePath}"
                            COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGC_BUILD__BIF_EMBEDDER_DIR}"
                            COMMAND "${PYTHON_EXECUTABLE}"
                            ARGS "${IGC_TOOLS_RESOURCE_LINKER}" "${embeddedFilePath}" "${_symbolFilePath}"
                                 "${symbolName}" "visibility"
                            DEPENDS ${IGC_TOOLS_RESOURCE_LINKER} ${_dependency} ${ARGN}
                            COMMENT "Linking Builtins"
                    )

  set("${symbolFilesVarName}" "${${symbolFilesVarName}}" "${_symbolFilePath}" PARENT_SCOPE)
endfunction()

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================

# ======================================================================================================
# ===================================== BUILD STEP CONFIGURATION =======================================
# ======================================================================================================

set(_oclResSymbolFiles)
set(_rsResSymbolFiles)
set(bifDepend ${IGC_BUILD__PROJ__BiFModule_OCL} ${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL})

igc_resource_embed_file(_oclResSymbolFiles _igc_bif_BC_120 "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc" ${bifDepend})
igc_resource_embed_file(_oclResSymbolFiles _igc_bif_BC_121 "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc" ${bifDepend})
igc_resource_embed_file(_oclResSymbolFiles _igc_bif_BC_122 "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc"   ${bifDepend})
# =========================================== Custom targets ============================================

set(IGC_BUILD__PROJ__BiFLib_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFLibOcl")
set(IGC_BUILD__PROJ__BiFLib_OCL       "${IGC_BUILD__PROJ__BiFLib_OCL}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__BiFLib_OCL "BiFLib-OCL")

add_library("${IGC_BUILD__PROJ__BiFLib_OCL}" STATIC
    ${_oclResSymbolFiles}
  )
set_property(TARGET "${IGC_BUILD__PROJ__BiFLib_OCL}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__BiFLib_OCL}")

# ============================================================================
# ============================================================================
# ============================================================================
