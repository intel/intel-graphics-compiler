;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S -dce -platformbmg -rev-id B -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vectorized_phi v_type=G type=f num_elts=16 align=dword
; CHECK: .decl vectorized_phi_wide v_type=G type=f num_elts=32 align=dword
; CHECK: exp (M1_NM, 16) vectorized_phi(0,0)<1> vectorized_phi(0,0)<1;1,0>

; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,0)<1> vectorized_phi_wide(0,0)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,1)<1> vectorized_phi_wide(0,1)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,2)<1> vectorized_phi_wide(0,2)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,3)<1> vectorized_phi_wide(0,3)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,4)<1> vectorized_phi_wide(0,4)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,5)<1> vectorized_phi_wide(0,5)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,6)<1> vectorized_phi_wide(0,6)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,7)<1> vectorized_phi_wide(0,7)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,8)<1> vectorized_phi_wide(0,8)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,9)<1> vectorized_phi_wide(0,9)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,10)<1> vectorized_phi_wide(0,10)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,11)<1> vectorized_phi_wide(0,11)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,12)<1> vectorized_phi_wide(0,12)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,13)<1> vectorized_phi_wide(0,13)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,14)<1> vectorized_phi_wide(0,14)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(0,15)<1> vectorized_phi_wide(0,15)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,0)<1> vectorized_phi_wide(1,0)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,1)<1> vectorized_phi_wide(1,1)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,2)<1> vectorized_phi_wide(1,2)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,3)<1> vectorized_phi_wide(1,3)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,4)<1> vectorized_phi_wide(1,4)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,5)<1> vectorized_phi_wide(1,5)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,6)<1> vectorized_phi_wide(1,6)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,7)<1> vectorized_phi_wide(1,7)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,8)<1> vectorized_phi_wide(1,8)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,9)<1> vectorized_phi_wide(1,9)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,10)<1> vectorized_phi_wide(1,10)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,11)<1> vectorized_phi_wide(1,11)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,12)<1> vectorized_phi_wide(1,12)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,13)<1> vectorized_phi_wide(1,13)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,14)<1> vectorized_phi_wide(1,14)<0;1,0>
; CHECK: exp (M1_NM, 1) vectorized_phi_wide(1,15)<1> vectorized_phi_wide(1,15)<0;1,0>

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {

entry:
  br label %bb1

bb1:                                      ; preds = %._crit_edge.._crit_edge_crit_edge, %6
  %vectorized_phi= phi <16 x float> [ zeroinitializer, %entry ], [ %vectorized_intrinsic, %bb1 ]
  %vectorized_phi_wide= phi <32 x float> [ zeroinitializer, %entry ], [ %vectorized_intrinsic_wide, %bb1 ]
  %vectorized_intrinsic = call <16 x float> @llvm.exp2.v16f32(<16 x float> %vectorized_phi)
  %vectorized_intrinsic_wide = call <32 x float> @llvm.exp2.v32f32(<32 x float> %vectorized_phi_wide)
  br i1 false, label %bb1, label %bb2

bb2:                                                ; preds = %._crit_edge
  %bitcast0 = bitcast <16 x float> %vectorized_intrinsic to <16 x i32>
  %bitcast1 = bitcast <32 x float> %vectorized_intrinsic_wide to <32 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <16 x i32> %bitcast0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <32 x i32> %bitcast1)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v16i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <16 x i32>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v32i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <32 x i32>)
declare <16 x float> @llvm.exp2.v16f32(<16 x float>)
declare <32 x float> @llvm.exp2.v32f32(<32 x float>)

!igc.functions = !{!0}

!0 = distinct !{void ()* @foo, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
