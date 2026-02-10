/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: system-windows
// REQUIRES: regkeys, pvc-supported

// Verify that generated LLVM IR dumps are not empty.

// RUN: rm -rf %t && mkdir %t
// RUN: ocloc compile -file %s -options "-igc_opts 'ShaderDumpEnableAll=1, DumpToCustomDir=%t, EnableStackCallFuncCall=1'" -device pvc
// RUN: test -z "$(find %t -type f -name "*.ll" -size 0c -print -quit 2>/dev/null)"

__attribute__((noinline))
double no_inline_func(double a) {
    return a + 39.0f;
}
__attribute__((intel_reqd_sub_group_size(32)))
kernel void foo(global double *a1, global double *a2) {
    int tid = get_global_id(0);
    a1[tid] = no_inline_func(a1[tid]);
}
