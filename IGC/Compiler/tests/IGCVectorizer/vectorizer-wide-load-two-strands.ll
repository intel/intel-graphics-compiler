;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; One <16 x float> load feeds two independent 8 wide strands. Neither strand
; covers the load exactly, so both repack into their own <8 x float>: the low
; one over elements 0..7, the high one over elements 8..15. The high strand
; starts at a non zero index, which is the contiguous window the swizzle check
; has to accept.

define spir_kernel void @test_wide_load_two_strands(ptr addrspace(1) %src, <8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %dst0, ptr addrspace(1) %dst1) {
; CHECK-LABEL: @test_wide_load_two_strands(
; CHECK:         [[LO0:%.*]] = insertelement <8 x float> undef, float %e0, i32 0
; CHECK:         [[LO7:%.*]] = insertelement <8 x float> {{.*}}, float %e7, i32 7
; CHECK:         [[LOMUL:%.*]] = fmul <8 x float> [[LO7]], {{.*}}
; CHECK:         [[HI0:%.*]] = insertelement <8 x float> undef, float %e8, i32 0
; CHECK:         [[HI7:%.*]] = insertelement <8 x float> {{.*}}, float %e15, i32 7
; CHECK:         [[HIMUL:%.*]] = fmul <8 x float> [[HI7]], {{.*}}
; CHECK:         call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[LOMUL]],
; CHECK:         call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[HIMUL]],
entry:
  %wide = load <16 x float>, ptr addrspace(1) %src, align 64
  %e0 = extractelement <16 x float> %wide, i64 0
  %e1 = extractelement <16 x float> %wide, i64 1
  %e2 = extractelement <16 x float> %wide, i64 2
  %e3 = extractelement <16 x float> %wide, i64 3
  %e4 = extractelement <16 x float> %wide, i64 4
  %e5 = extractelement <16 x float> %wide, i64 5
  %e6 = extractelement <16 x float> %wide, i64 6
  %e7 = extractelement <16 x float> %wide, i64 7
  %e8 = extractelement <16 x float> %wide, i64 8
  %e9 = extractelement <16 x float> %wide, i64 9
  %e10 = extractelement <16 x float> %wide, i64 10
  %e11 = extractelement <16 x float> %wide, i64 11
  %e12 = extractelement <16 x float> %wide, i64 12
  %e13 = extractelement <16 x float> %wide, i64 13
  %e14 = extractelement <16 x float> %wide, i64 14
  %e15 = extractelement <16 x float> %wide, i64 15

  %lo0 = fmul float %e0, 2.000000e+00
  %lo1 = fmul float %e1, 2.000000e+00
  %lo2 = fmul float %e2, 2.000000e+00
  %lo3 = fmul float %e3, 2.000000e+00
  %lo4 = fmul float %e4, 2.000000e+00
  %lo5 = fmul float %e5, 2.000000e+00
  %lo6 = fmul float %e6, 2.000000e+00
  %lo7 = fmul float %e7, 2.000000e+00

  %hi0 = fmul float %e8, 3.000000e+00
  %hi1 = fmul float %e9, 3.000000e+00
  %hi2 = fmul float %e10, 3.000000e+00
  %hi3 = fmul float %e11, 3.000000e+00
  %hi4 = fmul float %e12, 3.000000e+00
  %hi5 = fmul float %e13, 3.000000e+00
  %hi6 = fmul float %e14, 3.000000e+00
  %hi7 = fmul float %e15, 3.000000e+00

  %v0 = insertelement <8 x float> zeroinitializer, float %lo0, i64 0
  %v1 = insertelement <8 x float> %v0, float %lo1, i64 1
  %v2 = insertelement <8 x float> %v1, float %lo2, i64 2
  %v3 = insertelement <8 x float> %v2, float %lo3, i64 3
  %v4 = insertelement <8 x float> %v3, float %lo4, i64 4
  %v5 = insertelement <8 x float> %v4, float %lo5, i64 5
  %v6 = insertelement <8 x float> %v5, float %lo6, i64 6
  %v7 = insertelement <8 x float> %v6, float %lo7, i64 7

  %w0 = insertelement <8 x float> zeroinitializer, float %hi0, i64 0
  %w1 = insertelement <8 x float> %w0, float %hi1, i64 1
  %w2 = insertelement <8 x float> %w1, float %hi2, i64 2
  %w3 = insertelement <8 x float> %w2, float %hi3, i64 3
  %w4 = insertelement <8 x float> %w3, float %hi4, i64 4
  %w5 = insertelement <8 x float> %w4, float %hi5, i64 5
  %w6 = insertelement <8 x float> %w5, float %hi6, i64 6
  %w7 = insertelement <8 x float> %w6, float %hi7, i64 7

  %dpas0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %v7, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %w7, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  store <8 x float> %dpas0, ptr addrspace(1) %dst0, align 32
  store <8 x float> %dpas1, ptr addrspace(1) %dst1, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{void (ptr addrspace(1), <8 x i16>, <8 x i32>, ptr addrspace(1), ptr addrspace(1))* @test_wide_load_two_strands, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (ptr addrspace(1), <8 x i16>, <8 x i32>, ptr addrspace(1), ptr addrspace(1))* @test_wide_load_two_strands}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
