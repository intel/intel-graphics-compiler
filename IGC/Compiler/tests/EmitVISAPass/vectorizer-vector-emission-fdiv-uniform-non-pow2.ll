;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Uniform vector fdiv / inv whose width is not a power of two (12, 6, 3) has no
; matching execution size, so it can't be emitted as a single packed
; instruction.  It must be emitted per element; an exec size of 0 (what
; lanesToSIMDMode() yields for an unexpected lane count) is never valid.

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -S -dce -platformbmg -rev-id B -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK-NOT: (M1_NM, 0)

; CHECK: .decl vectorized_phi12 v_type=G type=f num_elts=12 align=dword
; CHECK: .decl vectorized_phi6 v_type=G type=f num_elts=6 align=dword
; CHECK: .decl vectorized_phi3 v_type=G type=f num_elts=3 align=dword

; CHECK: div (M1_NM, 1) vectorized_phi12(0,0)<1> vectorized_phi12(0,0)<0;1,0> vectorized_phi12(0,0)<0;1,0>
; CHECK-COUNT-10: div (M1_NM, 1) vectorized_phi12(0,{{[0-9]+}})<1>
; CHECK: div (M1_NM, 1) vectorized_phi12(0,11)<1> vectorized_phi12(0,11)<0;1,0> vectorized_phi12(0,11)<0;1,0>

; CHECK: inv (M1_NM, 1) vectorized_phi6(0,0)<1> vectorized_phi6(0,0)<0;1,0>
; CHECK-COUNT-4: inv (M1_NM, 1) vectorized_phi6(0,{{[0-9]+}})<1>
; CHECK: inv (M1_NM, 1) vectorized_phi6(0,5)<1> vectorized_phi6(0,5)<0;1,0>

; CHECK: div (M1_NM, 1) vectorized_phi3(0,0)<1> vectorized_phi3(0,0)<0;1,0> vectorized_phi3(0,0)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_phi3(0,1)<1> vectorized_phi3(0,1)<0;1,0> vectorized_phi3(0,1)<0;1,0>
; CHECK: div (M1_NM, 1) vectorized_phi3(0,2)<1> vectorized_phi3(0,2)<0;1,0> vectorized_phi3(0,2)<0;1,0>

; CHECK-NOT: (M1_NM, 0)

target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {
entry:
  br label %bb1

bb1:
  %vectorized_phi12 = phi <12 x float> [ zeroinitializer, %entry ], [ %vectorized_binary12, %bb1 ]
  %vectorized_phi6 = phi <6 x float> [ zeroinitializer, %entry ], [ %vectorized_binary6, %bb1 ]
  %vectorized_phi3 = phi <3 x float> [ zeroinitializer, %entry ], [ %vectorized_binary3, %bb1 ]
  %vectorized_binary12 = fdiv <12 x float> %vectorized_phi12, %vectorized_phi12
  %vectorized_binary6 = fdiv <6 x float> <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>, %vectorized_phi6
  %vectorized_binary3 = fdiv <3 x float> %vectorized_phi3, %vectorized_phi3
  br i1 false, label %bb1, label %bb2

bb2:
  %bitcast12 = bitcast <12 x float> %vectorized_binary12 to <12 x i32>
  %bitcast6 = bitcast <6 x float> %vectorized_binary6 to <6 x i32>
  %bitcast3 = bitcast <3 x float> %vectorized_binary3 to <3 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v12i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <12 x i32> %bitcast12)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v6i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <6 x i32> %bitcast6)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v3i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <3 x i32> %bitcast3)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v12i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <12 x i32>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v6i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <6 x i32>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v3i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <3 x i32>)

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @foo}
!6 = !{!"FuncMD", !5, !4}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
