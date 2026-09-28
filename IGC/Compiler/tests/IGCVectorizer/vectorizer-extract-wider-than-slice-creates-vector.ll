;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; The extract source %src is <16 x float>, wider than the 8 wide slice
; %e0..%e7. The source vector cannot be reused as is, so the seed is materialized
; with a fresh <8 x float> insertelement chain and the chain keeps vectorizing.

define spir_kernel void @test_extract_wider_than_slice(<16 x float> %wide, <8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_extract_wider_than_slice(
; CHECK:         [[V0:%.*]] = insertelement <8 x float> undef, float %e0, i32 0
; CHECK:         [[V7:%.*]] = insertelement <8 x float> {{.*}}, float %e7, i32 7
; CHECK:         [[MUL:%.*]] = fmul <8 x float> [[V7]], {{.*}}
; CHECK:         call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[MUL]],
entry:
  %src = fadd <16 x float> %wide, %wide
  %e0 = extractelement <16 x float> %src, i64 0
  %e1 = extractelement <16 x float> %src, i64 1
  %e2 = extractelement <16 x float> %src, i64 2
  %e3 = extractelement <16 x float> %src, i64 3
  %e4 = extractelement <16 x float> %src, i64 4
  %e5 = extractelement <16 x float> %src, i64 5
  %e6 = extractelement <16 x float> %src, i64 6
  %e7 = extractelement <16 x float> %src, i64 7
  %m0 = fmul float %e0, 2.000000e+00
  %m1 = fmul float %e1, 2.000000e+00
  %m2 = fmul float %e2, 2.000000e+00
  %m3 = fmul float %e3, 2.000000e+00
  %m4 = fmul float %e4, 2.000000e+00
  %m5 = fmul float %e5, 2.000000e+00
  %m6 = fmul float %e6, 2.000000e+00
  %m7 = fmul float %e7, 2.000000e+00
  %i0 = insertelement <8 x float> zeroinitializer, float %m0, i64 0
  %i1 = insertelement <8 x float> %i0, float %m1, i64 1
  %i2 = insertelement <8 x float> %i1, float %m2, i64 2
  %i3 = insertelement <8 x float> %i2, float %m3, i64 3
  %i4 = insertelement <8 x float> %i3, float %m4, i64 4
  %i5 = insertelement <8 x float> %i4, float %m5, i64 5
  %i6 = insertelement <8 x float> %i5, float %m6, i64 6
  %i7 = insertelement <8 x float> %i6, float %m7, i64 7
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %i7, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{void (<16 x float>, <8 x i16>, <8 x i32>, ptr addrspace(1))* @test_extract_wider_than_slice, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (<16 x float>, <8 x i16>, <8 x i32>, ptr addrspace(1))* @test_extract_wider_than_slice}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
