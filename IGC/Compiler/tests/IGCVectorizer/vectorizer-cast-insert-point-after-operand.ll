;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-vectorizer -S -dce --platformbmg < %s 2>&1 | FileCheck %s

; vectorized_cast must be placed after the vector it consumes, not before the
; first instruction of the slice.
;
; %x0 lives in a different BB than %x1 - %x7, so the operand vector for the
; fptrunc slice is built right after the minimal point of the slice itself
; (%t0), which is below the first slice element. Inserting the cast before %t0
; would then use the insertelement chain before it is defined.

; CHECK-LABEL: @k
; CHECK-LABEL: bb2:
; CHECK:      [[VEC0:%.*]] = insertelement <8 x float> undef, float %x0, i32 0
; CHECK-NEXT: [[VEC1:%.*]] = insertelement <8 x float> [[VEC0]], float %x1, i32 1
; CHECK-NEXT: [[VEC2:%.*]] = insertelement <8 x float> [[VEC1]], float %x2, i32 2
; CHECK-NEXT: [[VEC3:%.*]] = insertelement <8 x float> [[VEC2]], float %x3, i32 3
; CHECK-NEXT: [[VEC4:%.*]] = insertelement <8 x float> [[VEC3]], float %x4, i32 4
; CHECK-NEXT: [[VEC5:%.*]] = insertelement <8 x float> [[VEC4]], float %x5, i32 5
; CHECK-NEXT: [[VEC6:%.*]] = insertelement <8 x float> [[VEC5]], float %x6, i32 6
; CHECK-NEXT: [[VEC7:%.*]] = insertelement <8 x float> [[VEC6]], float %x7, i32 7
; CHECK-NEXT: %vectorized_cast = fptrunc <8 x float> [[VEC7]] to <8 x half>
; CHECK-NEXT: %v = bitcast <8 x half> %vectorized_cast to <8 x i16>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @k(float %p, <8 x i16> %a, <8 x i32> %b) {
entry:
  %x0 = fadd float %p, 1.000000e+00
  br label %bb2

bb2:
  %x1 = fadd float %p, 2.000000e+00
  %x2 = fadd float %p, 3.000000e+00
  %x3 = fadd float %p, 4.000000e+00
  %x4 = fadd float %p, 5.000000e+00
  %x5 = fadd float %p, 6.000000e+00
  %x6 = fadd float %p, 7.000000e+00
  %x7 = fadd float %p, 8.000000e+00

  %t0 = fptrunc float %x0 to half
  %t1 = fptrunc float %x1 to half
  %t2 = fptrunc float %x2 to half
  %t3 = fptrunc float %x3 to half
  %t4 = fptrunc float %x4 to half
  %t5 = fptrunc float %x5 to half
  %t6 = fptrunc float %x6 to half
  %t7 = fptrunc float %x7 to half

  %i0 = insertelement <8 x half> zeroinitializer, half %t0, i64 0
  %i1 = insertelement <8 x half> %i0, half %t1, i64 1
  %i2 = insertelement <8 x half> %i1, half %t2, i64 2
  %i3 = insertelement <8 x half> %i2, half %t3, i64 3
  %i4 = insertelement <8 x half> %i3, half %t4, i64 4
  %i5 = insertelement <8 x half> %i4, half %t5, i64 5
  %i6 = insertelement <8 x half> %i5, half %t6, i64 6
  %i7 = insertelement <8 x half> %i6, half %t7, i64 7
  %v = bitcast <8 x half> %i7 to <8 x i16>
  %d = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %v, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %d)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x float>)

!igc.functions = !{!0}
!0 = !{void (float, <8 x i16>, <8 x i32>)* @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (float, <8 x i16>, <8 x i32>)* @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
