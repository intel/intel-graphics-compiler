#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__igc_opt       "${IGC_BUILD__PROJ_NAME_PREFIX}igc_opt")
set(IGC_BUILD__PROJ__igc_opt       "${IGC_BUILD__PROJ__igc_opt}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__igc_opt "${IGC_BUILD__PROJ__igc_opt}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__igc_opt
    "${IGC_BUILD__IGC_SRC_DIR}/AdaptorOCL/OCL/LoadBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InitializeAllIGCPasses.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CreateIGCContext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RestoreGenISAIntrinsicDeclarations.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_opt.cpp"
  )

set(IGC_BUILD__HDR__igc_opt)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IGC_BUILD__TOOLS_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IGC_BUILD__TOOLS_OUTPUT_DIR})

add_executable("${IGC_BUILD__PROJ__igc_opt}"
    ${IGC_BUILD__SRC__igc_opt}
    ${IGC_BUILD__HDR__igc_opt}
    ${IGC_BUILD__RES__igc_opt}
  )
set_target_properties("${IGC_BUILD__PROJ__igc_opt}" PROPERTIES FOLDER "Tools")
target_include_directories("${IGC_BUILD__PROJ__igc_opt}" PRIVATE
    "${IGC_BUILD__IGC_SRC_DIR}/../../Tools/IGC"
    "${IGC_BUILD__IGC_SRC_DIR}/GenISAIntrinsics"
  )
set_property(TARGET "${IGC_BUILD__PROJ__igc_opt}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__igc_opt}")

igc_get_llvm_targets(LLVM_LIBS
  AggressiveInstCombine
  Analysis
  BinaryFormat
  BitReader
  BitStreamReader
  BitWriter
  CodeGen
  Core
  Demangle
  InstCombine
  Instrumentation
  IPO
  IRReader
  Linker
  MC
  MCParser
  Object
  ProfileData
  ScalarOpts
  Support
  Target
  TransformUtils
  Vectorize
  )

target_link_libraries("${IGC_BUILD__PROJ__igc_opt}"
    "${IGC_BUILD__LINK_LINE_RELEASE__igc_lib}"
    ${LLVM_LIBS}
  )

if(UNIX)
  target_link_libraries("${IGC_BUILD__PROJ__igc_opt}" dl)
endif()

if(UNIX)
  set(IGC_BUILD__PROJ__igc_opt_LINK_OPT )
  list(APPEND IGC_BUILD__PROJ__igc_opt_LINK_OPT "-Wl,--no-gc-sections")
  list(APPEND IGC_BUILD__PROJ__igc_opt_LINK_OPT "-Wl,--export-dynamic")

  set_target_properties(${IGC_BUILD__PROJ__igc_opt} PROPERTIES LINK_OPTIONS "${IGC_BUILD__PROJ__igc_opt_LINK_OPT}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "")
