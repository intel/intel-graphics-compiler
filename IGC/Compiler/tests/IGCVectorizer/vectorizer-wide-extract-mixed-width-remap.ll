;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s


define spir_kernel void @test_mixed_width(ptr addrspace(1) %src, ptr addrspace(1) %dst16, ptr addrspace(1) %dst8) {
; CHECK-LABEL: @test_mixed_width(
; the 8 wide strand vectorizes against its own repack of the wide extract
; CHECK:         [[GATHER8:%.*]] = insertelement <8 x float> {{.*}}, float %e7, i32 7
; CHECK:         [[FADD8:%.*]] = fadd <8 x float> [[GATHER8]], {{.*}}
; CHECK:         [[LANE0:%.*]] = extractelement <8 x float> [[FADD8]], i32 0
; the low lanes of the 16 wide gather are rewired to the new extracts, but the
; gather itself stays <16 x float> and keeps feeding the 16 wide fmul
; CHECK:         insertelement <16 x float> undef, float [[LANE0]], i32 0
; CHECK:         [[GATHER16:%.*]] = insertelement <16 x float> {{.*}}, float %x15, i32 15
; CHECK:         [[FMUL16:%.*]] = fmul <16 x float> [[GATHER16]], {{.*}}
; CHECK:         store <16 x float> [[FMUL16]],
; CHECK:         store <8 x float> [[FADD8]],
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

  %x0 = fadd float %e0, 1.000000e+00
  %x1 = fadd float %e1, 1.000000e+00
  %x2 = fadd float %e2, 1.000000e+00
  %x3 = fadd float %e3, 1.000000e+00
  %x4 = fadd float %e4, 1.000000e+00
  %x5 = fadd float %e5, 1.000000e+00
  %x6 = fadd float %e6, 1.000000e+00
  %x7 = fadd float %e7, 1.000000e+00
  %x8 = fsub float %e8, 1.000000e+00
  %x9 = fsub float %e9, 1.000000e+00
  %x10 = fsub float %e10, 1.000000e+00
  %x11 = fsub float %e11, 1.000000e+00
  %x12 = fsub float %e12, 1.000000e+00
  %x13 = fsub float %e13, 1.000000e+00
  %x14 = fsub float %e14, 1.000000e+00
  %x15 = fsub float %e15, 1.000000e+00

  %m0 = fmul float %x0, 2.000000e+00
  %m1 = fmul float %x1, 2.000000e+00
  %m2 = fmul float %x2, 2.000000e+00
  %m3 = fmul float %x3, 2.000000e+00
  %m4 = fmul float %x4, 2.000000e+00
  %m5 = fmul float %x5, 2.000000e+00
  %m6 = fmul float %x6, 2.000000e+00
  %m7 = fmul float %x7, 2.000000e+00
  %m8 = fmul float %x8, 2.000000e+00
  %m9 = fmul float %x9, 2.000000e+00
  %m10 = fmul float %x10, 2.000000e+00
  %m11 = fmul float %x11, 2.000000e+00
  %m12 = fmul float %x12, 2.000000e+00
  %m13 = fmul float %x13, 2.000000e+00
  %m14 = fmul float %x14, 2.000000e+00
  %m15 = fmul float %x15, 2.000000e+00

  %v0 = insertelement <16 x float> zeroinitializer, float %m0, i64 0
  %v1 = insertelement <16 x float> %v0, float %m1, i64 1
  %v2 = insertelement <16 x float> %v1, float %m2, i64 2
  %v3 = insertelement <16 x float> %v2, float %m3, i64 3
  %v4 = insertelement <16 x float> %v3, float %m4, i64 4
  %v5 = insertelement <16 x float> %v4, float %m5, i64 5
  %v6 = insertelement <16 x float> %v5, float %m6, i64 6
  %v7 = insertelement <16 x float> %v6, float %m7, i64 7
  %v8 = insertelement <16 x float> %v7, float %m8, i64 8
  %v9 = insertelement <16 x float> %v8, float %m9, i64 9
  %v10 = insertelement <16 x float> %v9, float %m10, i64 10
  %v11 = insertelement <16 x float> %v10, float %m11, i64 11
  %v12 = insertelement <16 x float> %v11, float %m12, i64 12
  %v13 = insertelement <16 x float> %v12, float %m13, i64 13
  %v14 = insertelement <16 x float> %v13, float %m14, i64 14
  %v15 = insertelement <16 x float> %v14, float %m15, i64 15
  store <16 x float> %v15, ptr addrspace(1) %dst16, align 64

  %w0 = insertelement <8 x float> zeroinitializer, float %x0, i64 0
  %w1 = insertelement <8 x float> %w0, float %x1, i64 1
  %w2 = insertelement <8 x float> %w1, float %x2, i64 2
  %w3 = insertelement <8 x float> %w2, float %x3, i64 3
  %w4 = insertelement <8 x float> %w3, float %x4, i64 4
  %w5 = insertelement <8 x float> %w4, float %x5, i64 5
  %w6 = insertelement <8 x float> %w5, float %x6, i64 6
  %w7 = insertelement <8 x float> %w6, float %x7, i64 7
  store <8 x float> %w7, ptr addrspace(1) %dst8, align 32
  ret void
}

!igc.functions = !{!0}
!0 = !{void (ptr addrspace(1), ptr addrspace(1), ptr addrspace(1))* @test_mixed_width, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (ptr addrspace(1), ptr addrspace(1), ptr addrspace(1))* @test_mixed_width}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
