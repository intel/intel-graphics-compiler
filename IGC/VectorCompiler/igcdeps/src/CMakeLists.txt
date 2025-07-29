#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

vc_get_absolute_paths(SRC ${CMAKE_CURRENT_SOURCE_DIR}
  cmc.cpp
  PatchTokens.cpp
  ShaderDump.cpp
  ShaderOverride.cpp
  TranslationInterface.cpp
  )

target_sources(VCIGCDeps
  PRIVATE
    ${SRC}
  )
