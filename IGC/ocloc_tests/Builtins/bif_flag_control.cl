/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported, tgllp-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=BIFFlagCtrlResolution'" -device tgllp 2>&1 | FileCheck %s --check-prefix=CHECK-BASE
// RUN: ocloc compile -file %s -options " -cl-fast-relaxed-math -igc_opts 'PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=BIFFlagCtrlResolution'" -device tgllp 2>&1 | FileCheck %s --check-prefix=CHECK-FastRelaxedMath
// RUN: ocloc compile -file %s -options " -igc_opts 'PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=BIFFlagCtrlResolution'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-HasHWLocalThreadID

// Check for the flag FastRelaxedMath
// Check in base run if we have setup to false FastRelaxedMath flag
// CHECK-BASE: @__bif_flag_FastRelaxedMath = local_unnamed_addr addrspace(2) constant i8 0,
// Check in run with option -cl-fast-relaxed-math we have setup to true FastRelaxedMath flag
// CHECK-FastRelaxedMath: @__bif_flag_FastRelaxedMath = local_unnamed_addr addrspace(2) constant i8 1,


// Check for the flag hasHWLocalThreadID
// Check in base run if we have setup to true hasHWLocalThreadID flag
// CHECK-BASE: @__bif_flag_hasHWLocalThreadID = local_unnamed_addr addrspace(2) constant i8 0,
// Check in run with pvc compilation if we have setup to true hasHWLocalThreadID flag
// CHECK-HasHWLocalThreadID: @__bif_flag_hasHWLocalThreadID = local_unnamed_addr addrspace(2) constant i8 1,

kernel void test(global float* out, global float* in)
{
    if(get_sub_group_id() == 0)
    {
       out[0] = acosh(in[0]);
    }
}
