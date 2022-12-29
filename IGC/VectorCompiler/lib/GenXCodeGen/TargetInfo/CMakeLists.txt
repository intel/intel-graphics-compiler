#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(INFO_SOURCES
  GenXTargetInfo.cpp
)

add_library(VCTargetInfo OBJECT ${INFO_SOURCES})

igc_get_llvm_targets(LLVM_LIBS Support)

target_link_libraries(VCTargetInfo ${LLVM_LIBS})
