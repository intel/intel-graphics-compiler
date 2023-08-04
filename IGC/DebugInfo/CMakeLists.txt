#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(DEBUG_INFO_LIBRARY_SOURCES
  StreamEmitter.cpp
  DIE.cpp
  DwarfCompileUnit.cpp
  LexicalScopes.cpp
  DwarfDebug.cpp
  VISADebugInfo.cpp
  VISADebugEmitter.cpp
  VISADebugDecoder.cpp
  VISAModule.cpp
  Utils.cpp
)

set(DEBUG_INFO_LIBRARY_HEADERS
  StreamEmitter.hpp
  DIE.hpp
  DwarfCompileUnit.hpp
  LexicalScopes.hpp
  DwarfDebug.hpp
  VISADebugInfo.hpp
  VISADebugEmitter.hpp
  VISADebugDecoder.hpp
  VISAModule.hpp
  Utils.hpp
)

add_library(GenXDebugInfo STATIC
  ${DEBUG_INFO_LIBRARY_SOURCES}
  ${DEBUG_INFO_LIBRARY_HEADERS})
add_dependencies(GenXDebugInfo intrinsics_gen ${IGC_BUILD__PROJ__GenISAIntrinsics})

igc_get_llvm_targets(LLVM_LIBS
  BinaryFormat
  Core
  MC
  Support
  )

target_link_libraries(GenXDebugInfo
  ${LLVM_LIBS}
  )
