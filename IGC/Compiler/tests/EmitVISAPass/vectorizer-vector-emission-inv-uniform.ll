;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Uniform vector fdiv with an all-ones numerator is emitted as inv. It must be a
; single packed inv when the vector fits into the SIMD width, and fall back to
; per-element emission otherwise.

; REQUIRES: regkeys
; RUN: igc_opt -S -dce -platformbmg -rev-id B -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vectorized_phi v_type=G type=f num_elts=16 align=dword
; CHECK: .decl vectorized_phi_wide v_type=G type=f num_elts=32 align=dword
; CHECK-NOT: div (
; CHECK: inv (M1_NM, 16) vectorized_phi(0,0)<1> vectorized_phi(0,0)<1;1,0>
; CHECK-NOT: inv (M1_NM, 1) vectorized_phi(
; CHECK: inv (M1_NM, 1) vectorized_phi_wide(0,0)<1> vectorized_phi_wide(0,0)<0;1,0>
; CHECK-COUNT-14: inv (M1_NM, 1) vectorized_phi_wide(0,{{[0-9]+}})<1>
; CHECK: inv (M1_NM, 1) vectorized_phi_wide(0,15)<1> vectorized_phi_wide(0,15)<0;1,0>
; CHECK: inv (M1_NM, 1) vectorized_phi_wide(1,0)<1> vectorized_phi_wide(1,0)<0;1,0>
; CHECK-COUNT-14: inv (M1_NM, 1) vectorized_phi_wide(1,{{[0-9]+}})<1>
; CHECK: inv (M1_NM, 1) vectorized_phi_wide(1,15)<1> vectorized_phi_wide(1,15)<0;1,0>
; CHECK-NOT: inv (M1_NM, 32)
; CHECK-NOT: div (
; CHECK: inv (M1_NM, 16) vectorized_phi_neg(0,0)<1> (-)vectorized_phi_neg(0,0)<1;1,0>

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {
entry:
  br label %bb1

bb1:
  %vectorized_phi = phi <16 x float> [ zeroinitializer, %entry ], [ %vectorized_binary, %bb1 ]
  %vectorized_phi_neg = phi <16 x float> [ zeroinitializer, %entry ], [ %vectorized_binary_neg, %bb1 ]
  %vectorized_phi_wide = phi <32 x float> [ zeroinitializer, %entry ], [ %vectorized_binary_wide, %bb1 ]
  %vectorized_binary = fdiv <16 x float> <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>, %vectorized_phi
  %vectorized_binary_wide = fdiv <32 x float> <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>, %vectorized_phi_wide
  %neg = fneg <16 x float> %vectorized_phi_neg
  %vectorized_binary_neg = fdiv <16 x float> <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>, %neg
  br i1 false, label %bb1, label %bb2

bb2:
  %bitcast0 = bitcast <16 x float> %vectorized_binary to <16 x i32>
  %bitcast2 = bitcast <16 x float> %vectorized_binary_neg to <16 x i32>
  %bitcast1 = bitcast <32 x float> %vectorized_binary_wide to <32 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <16 x i32> %bitcast0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <32 x i32> %bitcast1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <16 x i32> %bitcast2)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v16i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <16 x i32>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v32i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <32 x i32>)

!igc.functions = !{!0}

!0 = distinct !{void ()* @foo, !1}
!1 = distinct !{!2}
!2 = distinct !{!"function_type", i32 0}

!3 = distinct !{!"requiredSubGroupSize", i32 16}
!4 = distinct !{!"FuncMDValue[0]", !3}
!5 = distinct !{!"FuncMDMap[0]", void ()* @foo}
!6 = distinct !{!"FuncMD", !5, !4}
!7 = distinct !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
