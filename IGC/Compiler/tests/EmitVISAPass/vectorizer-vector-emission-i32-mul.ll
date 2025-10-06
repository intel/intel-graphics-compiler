;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S -dce -platformbmg -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vectorized_phi v_type=G type=d num_elts=128 align=wordx32
; CHECK: .decl vector v_type=G type=d num_elts=8 align=dword

; CHECK: mul (M1, 16) vectorized_phi(0,0)<1> vector(0,0)<0;1,0> vectorized_phi(0,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(1,0)<1> vector(0,1)<0;1,0> vectorized_phi(1,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(2,0)<1> vector(0,2)<0;1,0> vectorized_phi(2,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(3,0)<1> vector(0,3)<0;1,0> vectorized_phi(3,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(4,0)<1> vector(0,4)<0;1,0> vectorized_phi(4,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(5,0)<1> vector(0,5)<0;1,0> vectorized_phi(5,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(6,0)<1> vector(0,6)<0;1,0> vectorized_phi(6,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(7,0)<1> vector(0,7)<0;1,0> vectorized_phi(7,0)<1;1,0>


define spir_kernel void @_foo() {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.._crit_edge_crit_edge, %6
  %1 = phi float [ 0.000000e+00, %0 ], [ %1, %._crit_edge.._crit_edge_crit_edge ]
  %vectorized_phi = phi <8 x i32> [ zeroinitializer, %0 ], [ %2, %._crit_edge.._crit_edge_crit_edge ]
  %vector = insertelement <8 x i32> zeroinitializer, i32 0, i64 0
  %vectorized_binary = mul <8 x i32> %vector, %vectorized_phi
  %2 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x i32> %vectorized_binary, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge.._crit_edge_crit_edge

._crit_edge.._crit_edge_crit_edge:                ; preds = %._crit_edge
  br label %._crit_edge
}

declare <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x i32>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}

!0 = !{void ()* @_foo, !1}
!1 = !{!2, !4, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{!"max_reg_pressure", i32 185}
