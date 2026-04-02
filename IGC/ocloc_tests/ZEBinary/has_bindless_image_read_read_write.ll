;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks if has_bindless_image_read zeinfo flag is properly generated
; for bindless image modules with __read_write images.
;
; This test has been reduced from the following opencl code:
;
; // RUN: ocloc compile -file %s -options "-cl-std=CL3.0" -device dg2
;
; __kernel void test(__read_write image3d_t input,
;                    __global uchar *dst,
;                    int4 srcOffset,
;                    uint dstOffset,
;                    uint2 Pitch) {
;     uint x = get_global_id(0);
;     uint y = get_global_id(1);
;     uint z = get_global_id(2);
;
;     const int4 srcCoord = (int4)(x, y, z, 0) + srcOffset;
;     uint DstOffset = dstOffset + (y * Pitch.x) + (z * Pitch.y);
;
;     uint4 c = read_imageui(input, srcCoord);
;     *(dst + DstOffset + x) = convert_uchar_sat(c.x);
; }
;
; Then the test was passed through llvm-reduce.

; REQUIRES: llvm-as, llvm-spirv, regkeys, llvm-14-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %OPAQUE_PTR_FLAG% %t.bc --spirv-ext=SPV_INTEL_bindless_images -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-cl-std=CL3.0 -igc_opts 'DumpZEInfoToConsole=1'" \
; RUN: -internal_options "-cl-intel-use-bindless-mode" -device dg2 | FileCheck %s
; CHECK:     has_bindless_image_read:

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test() {
entry:
  %call11 = call spir_func <4 x i32> @_Z24__spirv_ImageRead_Ruint4PU3AS133__spirv_Image__void_2_0_0_0_0_0_2Dv4_ii(target("spirv.Image", void, 2, 0, 0, 0, 0, 0, 2) zeroinitializer, <4 x i32> zeroinitializer, i32 0)
  ret void
}

declare spir_func <4 x i32> @_Z24__spirv_ImageRead_Ruint4PU3AS133__spirv_Image__void_2_0_0_0_0_0_2Dv4_ii(target("spirv.Image", void, 2, 0, 0, 0, 0, 0, 2), <4 x i32>, i32)
