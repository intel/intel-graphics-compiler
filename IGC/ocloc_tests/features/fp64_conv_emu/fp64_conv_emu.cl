/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options "-cl-fp64-gen-conv-emu -igc_opts 'PrintToConsole=1 PrintAfter=PreCompiledFuncImport'" -internal_options "-cl-ext=-all,+cl_khr_fp64" 2>&1 | FileCheck %s --check-prefix=CHECK-BASE

// CHECK-LABEL: @conversion_kernel(
// CHECK-BASE: entry:
// CHECK-BASE:  [[DPEmuFlag:%.*]] = alloca i32, align 4
// CHECK-BASE:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 %{{.*}}
// CHECK-BASE:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
// CHECK-BASE:  [[CALL_FTMP:%.*]] = call i32 @__igcbuiltin_dp_to_int32(double [[TMP3]], i32 3, i32 0, i32* [[DPEmuFlag]])
// CHECK-BASE:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %{{.*}}
// CHECK-BASE:  store i32 [[CALL_FTMP]], i32 addrspace(1)* [[ARRAY_IDX2]], align 4
// CHECK-BASE:  ret void

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void conversion_kernel(__global double* inA, __global uint* out)
{
    size_t id = get_global_id(0);
    out[id] = convert_int(inA[id]);
}
