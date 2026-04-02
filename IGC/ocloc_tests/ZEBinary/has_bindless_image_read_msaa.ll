;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks if has_bindless_image_read zeinfo flag is properly generated
; for bindless image modules with msaa images.
;
; This test has been reduced from the following opencl code:
;
; // RUN: ocloc compile -file %s -options "-cl-std=CL3.0" -device dg2 \
; // RUN: -internal_options "-cl-ext=+cl_khr_gl_msaa_sharing"
;
; __kernel void test(read_only  image2d_array_msaa_t src,
;                    write_only image2d_array_t      dst,
;                    int width,
;                    int height,
;                    int layers,
;                    int sampleIndex) {
;     const int x = (int)get_global_id(0);
;     const int y = (int)get_global_id(1);
;     const int layer = (int)get_global_id(2);
;
;     if (x >= width || y >= height || layer >= layers) return;
;
;     const int4 coord = (int4)(x, y, layer, 0);
;
;     const float4 c = read_imagef(src, coord, sampleIndex);
;     write_imagef(dst, coord, c);
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

define spir_kernel void @test(target("spirv.Image", void, 1, 0, 1, 1, 0, 0, 0) %src) {
entry:
  %call14 = call spir_func <4 x float> @_Z25__spirv_ImageRead_Rfloat4PU3AS133__spirv_Image__void_1_0_1_1_0_0_0Dv4_iii(target("spirv.Image", void, 1, 0, 1, 1, 0, 0, 0) %src, <4 x i32> zeroinitializer, i32 0, i32 0)
  ret void
}

declare spir_func <4 x float> @_Z25__spirv_ImageRead_Rfloat4PU3AS133__spirv_Image__void_1_0_1_1_0_0_0Dv4_iii(target("spirv.Image", void, 1, 0, 1, 1, 0, 0, 0), <4 x i32>, i32, i32)
