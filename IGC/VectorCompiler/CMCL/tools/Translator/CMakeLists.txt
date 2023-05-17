#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

igc_imported_tool(CMCLTranslatorTool CMCLTranslatorTool)

if(NOT TARGET CMCLTranslatorTool)
  add_executable(CMCLTranslatorTool Main.cpp)

  # LLVM dependencies
  igc_get_llvm_targets(LLVM_LIBS
    Analysis
    AsmParser
    BinaryFormat
    BitReader
    BitstreamReader
    CodeGen
    Core
    IRReader
    MC
    MCParser
    Object
    ProfileData
    Remarks
    Support
  # Not really required but VS linker fails on its dependency from Support
    Demangle
    BitWriter
    )

  target_link_libraries(CMCLTranslatorTool
    CMCLSupport
    ${LLVM_LIBS})
endif()
