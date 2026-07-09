;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S %s -opaque-pointers -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 32 &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK: .decl vectorized_phi v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vectorized_phi71 v_type=G type=f num_elts=128 align=wordx32
; CHECK: .kernel_attr SimdSize=32

; CHECK: mov (M1, 32) vectorized_phi(0,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi(2,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi(4,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi(6,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi71(0,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi71(2,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi71(4,0)<1> 0x0:f
; CHECK: mov (M1, 32) vectorized_phi71(6,0)<1> 0x0:f

; CHECK: dpas.bf.bf.8.8 (M1, 16) vectorized_phi.0
; CHECK: dpas.bf.bf.8.8 (M1, 16) vectorized_phi71.0
; CHECK: dpas.bf.bf.8.8 (M1, 16) vectorized_phi.0
; CHECK: dpas.bf.bf.8.8 (M1, 16) vectorized_phi71.0

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {
bb:
  br label %bb79

bb79:                                             ; preds = %bb79, %bb
  %vectorized_phi = phi <4 x float> [ zeroinitializer, %bb ], [ %i264, %bb79 ]
  %vectorized_phi71 = phi <4 x float> [ zeroinitializer, %bb ], [ %i265, %bb79 ]
  %i248 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> %vectorized_phi, <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 11, i32 11, i32 8, i32 8, i1 false)
  %i249 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> %vectorized_phi71, <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 11, i32 11, i32 8, i32 8, i1 false)
  %i264 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> %i248, <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 11, i32 11, i32 8, i32 8, i1 false)
  %i265 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float> %i249, <4 x i16> zeroinitializer, <4 x i32> zeroinitializer, i32 11, i32 11, i32 8, i32 8, i1 false)
  br label %bb79
}

declare <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v4i32(<4 x float>, <4 x i16>, <4 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}

!0 = distinct !{ptr @foo, !1}
!1 = distinct !{!2}
!2 = distinct !{!"function_type", i32 0}
