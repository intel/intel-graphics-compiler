#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(LLVM_TARGET_DEFINITIONS ApiOptions.td)
igc_tablegen(LLVM ApiOptions.inc -gen-opt-parser-defs)

set(LLVM_TARGET_DEFINITIONS InternalOptions.td)
igc_tablegen(LLVM InternalOptions.inc -gen-opt-parser-defs)

add_public_tablegen_target(IGCOptionsTablegen)
