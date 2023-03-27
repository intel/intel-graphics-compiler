;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-madloopslice -S < %s | FileCheck %s
; ------------------------------------------------
; MadLoopSlice
; ------------------------------------------------

define spir_kernel void @test() {
entry:
  br label %for.body

; CHECK: %c0 = call double @llvm.fma.f64(double %a0, double %b0, double 1.000000e+00)
; CHECK-NEXT: %d0 = call double @llvm.fma.f64(double %b0, double %c0, double 1.000000e+00)
; CHECK-NEXT: %c1 = call double @llvm.fma.f64(double %a1, double %b1, double 1.000000e+00)
; CHECK-NEXT: %d1 = call double @llvm.fma.f64(double %b1, double %c1, double 1.000000e+00)

for.body:                                         ; preds = %for.body, %entry
  %i = phi i32 [ 1, %entry ], [ %iter, %for.body ]
  %a0 = phi double [ 1.000000e+00, %entry ], [ %c0, %for.body ]
  %b0 = phi double [ 0.000000e+00, %entry ], [ %d0, %for.body ]
  %a1 = phi double [ 1.000000e+00, %entry ], [ %c1, %for.body ]
  %b1 = phi double [ 0.000000e+00, %entry ], [ %d1, %for.body ]
  %c0 = call double @llvm.fma.f64(double %a0, double %b0, double 1.000000e+00)
  %c1 = call double @llvm.fma.f64(double %a1, double %b1, double 1.000000e+00)
  %d0 = call double @llvm.fma.f64(double %b0, double %c0, double 1.000000e+00)
  %d1 = call double @llvm.fma.f64(double %b1, double %c1, double 1.000000e+00)
  %iter = add nuw nsw i32 %i, 1
  %cmp = icmp ult i32 %iter, 100
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
}

declare double @llvm.fma.f64(double, double, double)

!igc.functions = !{!0}

!0 = !{void ()* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
