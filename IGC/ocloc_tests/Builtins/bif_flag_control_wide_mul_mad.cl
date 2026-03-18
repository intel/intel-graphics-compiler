/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// RUN: %if pvc-supported %{ ocloc compile -file %s -options " -igc_opts 'PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=BIFFlagCtrlResolution'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-BASE %}
// RUN: %if cri-supported %{ ocloc compile -file %s -options " -igc_opts 'EnableWideMulMad=1 PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=BIFFlagCtrlResolution'" -device cri 2>&1 | FileCheck %s --check-prefix=CHECK-HasWideMulMad %}


// Check for the flag HasWideMulMad (this test also checks mangling of igc name flags)
// Check in base run if we have setup to false HasWideMulMad flag
// CHECK-BASE: @__bif_flag_HasWideMulMad = local_unnamed_addr addrspace(2) constant i8 0
// Check in run with option EnableWideMulMad and for CRI platform if we have setup to true HasWideMulMad flag
// CHECK-HasWideMulMad: @__bif_flag_HasWideMulMad = local_unnamed_addr addrspace(2) constant i8 1

kernel void test(global long* out, global long* in)
{
    out[0] = mad_sat(in[0], in[1], in[2]);
}
