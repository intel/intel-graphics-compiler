/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options "-cl-fp64-gen-conv-emu -igc_opts 'PrintToConsole=1 PrintAfter=PreCompiledFuncImport'" -internal_options "-cl-ext=-all,+cl_khr_fp64" 2>&1 | FileCheck %s --check-prefix=CHECK-BASE

// CHECK-LABEL: @fcmp_kernel(
// CHECK-BASE: entry:
// CHECK-BASE:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 %{{.*}}
// CHECK-BASE:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
// CHECK-BASE:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %inB, i64 %{{.*}}
// CHECK-BASE:  [[TMP4:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX1]], align 8
// CHECK-BASE:  [[CALL_FTMP:%.*]] = call i32 @__igcbuiltin_dp_cmp(double [[TMP3]], double [[TMP4]], i32 0)
// CHECK-BASE:  [[SHL:%.*]] = shl i32 1, [[CALL_FTMP]]
// CHECK-BASE:  [[AND:%.*]] = and i32 4, [[SHL]]
// CHECK-BASE:  [[DPEmuCmp:%.*]] = icmp ne i32 [[AND]], 0
// CHECK-BASE:  br i1 [[DPEmuCmp]], label %if.then, label %if.else
// CHECK-BASE: if.then:
// CHECK-BASE:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 %{{.*}}
// CHECK-BASE:  store double [[TMP3]], double addrspace(1)* [[ARRAY_IDX2]], align 8
// CHECK-BASE:  br label %if.end
// CHECK-BASE: if.else:
// CHECK-BASE:  [[ARRAY_IDX3:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 %{{.*}}
// CHECK-BASE:  store double [[TMP4]], double addrspace(1)* [[ARRAY_IDX3]], align 8
// CHECK-BASE:  br label %if.end
// CHECK-BASE: if.end:
// CHECK-BASE:  ret void

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void fcmp_kernel(__global double* inA, __global double* inB, __global double* out)
{
    size_t id = get_global_id(0);
    if (inA[id] > inB[id])
    {
        out[id] = inA[id];
    }
    else
    {
        out[id] = inB[id];
    }
}
