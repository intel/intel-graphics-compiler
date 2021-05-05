#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(LLVM_TARGET_DEFINITIONS Options.td)
igc_tablegen(LLVM Options.inc -gen-opt-parser-defs)
add_public_tablegen_target(IGCOptionsTablegen)
