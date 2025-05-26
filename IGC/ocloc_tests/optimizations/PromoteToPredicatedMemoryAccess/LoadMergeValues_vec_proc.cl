/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// tests below verify different cases of transforming merge value for predicated loads
// like load <8xi8> -> load <2 x i32>

// REQUIRES: regkeys,pvc-supported,llvm-16-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnableOpaquePointersBackend=1 EnablePromoteToPredicatedMemoryAccess=1 PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s

// CHECK: define spir_kernel void @test_vec_process_load_splat
// CHECK: [[BC1:%.*]] = bitcast <8 x i8> <i8 5, i8 5, i8 5, i8 5, i8 5, i8 5, i8 5, i8 5> to <2 x i32>
// CHECK: call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) {{%.*}}, i64 8, i1 {{%.*}}, <2 x i32> [[BC1]])
__kernel void test_vec_process_load_splat(__global const char8* in, __global char8* out, const int predicate) {
    int gid = get_global_id(0);
    char8 val = (char8){5, 5, 5, 5, 5, 5, 5, 5};
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK: define spir_kernel void @test_vec_process_load_const
// CHECK: [[BC2:%.*]] = bitcast <8 x i8> <i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8> to <2 x i32>
// CHECK: call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) {{%.*}}, i64 8, i1 {{%.*}}, <2 x i32> [[BC2]])
__kernel void test_vec_process_load_const(__global const char8* in, __global char8* out, const int predicate) {
    int gid = get_global_id(0);
    char8 val = (char8){1, 2, 3, 4, 5, 6, 7, 8};
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK: define spir_kernel void @test_vec_process_load_var
// CHECK: [[VEC:%.*]] = insertelement <8 x i8> {{%.*}}, i8 {{%.*}}, i64 7
// CHECK: [[BC3:%.*]] = bitcast <8 x i8> [[VEC]] to <2 x i32>
// CHECK: call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) {{%.*}}, i64 8, i1 {{%.*}}, <2 x i32> [[BC3]])
__kernel void test_vec_process_load_var(__global const char8* in, __global char8* out, const int predicate) {
     int gid = get_global_id(0);
     char8 val = (char8){gid + 1, gid + 2, gid + 3, gid + 4, gid + 5, gid + 6, gid + 7, gid + 8};
     if (gid <= predicate)
         val = in[gid];
     out[gid] = val;
}
