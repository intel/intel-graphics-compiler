# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# Wrapper around linker command that filters out LLVM libraries
# for plugin linking.

from sys import argv
from subprocess import check_call

def is_llvm_library(arg):
    if 'LLVMGenXIntrinsics' in arg:
        return False
    if 'LLVM' in arg:
        return True
    return False

args = [arg for arg in argv[1:] if not is_llvm_library(arg)]
check_call(args)
