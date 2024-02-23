/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL2.0 -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s --check-prefix=CHECK-BASE
// RUN: ocloc compile -file %s -device pvc -options "-cl-std=CL2.0 -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s --check-prefix=CHECK-PVC

// The reduction for PVC is more unrolled
// CHECK-BASE-COUNT-3: call float @llvm.genx.GenISA.WaveAll
// CHECK-PVC-COUNT-4: call float @llvm.genx.GenISA.WaveAll

kernel void test(__global float* out, __global float* in)
{
    in[0] = work_group_reduce_add(out[get_local_id(0)]);
}
