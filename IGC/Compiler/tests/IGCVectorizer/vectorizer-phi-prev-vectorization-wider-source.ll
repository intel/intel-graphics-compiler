;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; Phi now has the same heuristic and checks for wider prev vectorization

define spir_kernel void @test_narrow_phi_over_wider_source(ptr addrspace(3) %d4, ptr addrspace(3) %d2) {
; CHECK-LABEL: @test_narrow_phi_over_wider_source(
; CHECK:       loop:
; CHECK:         [[NARROW_PHI:%[a-zA-Z0-9_.]+]] = phi <2 x float> [ zeroinitializer, %entry ], [ {{.*}}, %loop ]
; CHECK:         [[WIDE_PHI:%[a-zA-Z0-9_.]+]] = phi <4 x float> [ {{.*}}, %entry ], [ [[WIDE_VEC:%[a-zA-Z0-9_.]+]], %loop ]

; CHECK:         [[E0:%[a-zA-Z0-9_.]+]] = extractelement <2 x float> [[NARROW_PHI]], i32 0
; CHECK:         [[E1:%[a-zA-Z0-9_.]+]] = extractelement <2 x float> [[NARROW_PHI]], i32 1
; CHECK:         insertelement <4 x float> undef, float [[E0]], i32 0
; CHECK:         insertelement <4 x float> {{.*}}, float [[E1]], i32 1
; CHECK:         [[WIDE_VEC]] = insertelement <4 x float> {{.*}}, float %p3, i32 3
; CHECK:         fmul <4 x float> [[WIDE_PHI]], {{%[a-zA-Z0-9_.]+}}
entry:
  br label %loop

loop:                                             ; preds = %loop, %entry
  %p0 = phi float [ 0.000000e+00, %entry ], [ %x0, %loop ]
  %p1 = phi float [ 0.000000e+00, %entry ], [ %x1, %loop ]
  %p2 = phi float [ 0.000000e+00, %entry ], [ %x2, %loop ]
  %p3 = phi float [ 0.000000e+00, %entry ], [ %x3, %loop ]
  %t0 = phi float [ 1.000000e+00, %entry ], [ %p0, %loop ]
  %t1 = phi float [ 1.000000e+00, %entry ], [ %p1, %loop ]
  %t2 = phi float [ 1.000000e+00, %entry ], [ %p2, %loop ]
  %t3 = phi float [ 1.000000e+00, %entry ], [ %p3, %loop ]
  %y1 = fadd float %p1, 1.000000e+00
  %y2 = fadd float %p2, 1.000000e+00
  %y3 = fadd float %p3, 1.000000e+00
  %a0 = fmul float %t0, %p0
  %a1 = fmul float %t1, %y1
  %a2 = fmul float %t2, %y2
  %a3 = fmul float %t3, %y3
  %w0 = insertelement <4 x float> zeroinitializer, float %a0, i64 0
  %w1 = insertelement <4 x float> %w0, float %a1, i64 1
  %w2 = insertelement <4 x float> %w1, float %a2, i64 2
  %w3 = insertelement <4 x float> %w2, float %a3, i64 3
  store <4 x float> %w3, ptr addrspace(3) %d4, align 16
  %q0 = insertelement <2 x float> zeroinitializer, float %p0, i64 0
  %q1 = insertelement <2 x float> %q0, float %p1, i64 1
  store <2 x float> %q1, ptr addrspace(3) %d2, align 8
  %x0 = fadd float %a0, 1.000000e+00
  %x1 = fadd float %a1, 1.000000e+00
  %x2 = fadd float %a2, 1.000000e+00
  %x3 = fadd float %a3, 1.000000e+00
  br label %loop
}

!igc.functions = !{!0}
!0 = !{void (ptr addrspace(3), ptr addrspace(3))* @test_narrow_phi_over_wider_source, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (ptr addrspace(3), ptr addrspace(3))* @test_narrow_phi_over_wider_source}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
