;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify -igc-custom-loop-opt -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CustomLoopVersioning
; ------------------------------------------------

; This test checks that we're not creating a cmp inst with mismatched fp types.

define spir_kernel void @test_customloop(ptr addrspace(65549) %a, double %b, double %c, double %d) {
entry:
  %aa = inttoptr i32 42 to ptr addrspace(65549)
  %cc = call double @llvm.maxnum.f64(double %c, double 3.000000e+00)
  %dd = call double @llvm.minnum.f64(double %d, double 2.000000e+00)
  br label %pre_header

pre_header:
  ; CHECK: [[TMP:%.*]] = load double
  %0 = load double, ptr addrspace(65549) %aa, align 4
  %1 = fmul double %b, %0
  ; CHECK: fcmp fast ogt double [[TMP]], 1.000000e+00
  br label %loop_body

loop_body:
  %2 = phi double [ %b, %pre_header ], [ %3, %loop_body ]
  %3 = phi double [ %1, %pre_header ], [ %7, %loop_body ]
  %4 = call double @llvm.maxnum.f64(double %cc, double %2)
  %5 = call double @llvm.minnum.f64(double %dd, double %3)
  %6 = load double, ptr addrspace(65549) %aa, align 4
  %7 = fmul double %3, %6
  %8 = fcmp ult double %3, %dd
  br i1 %8, label %loop_body, label %end

end:
  store double %3, ptr addrspace(65549) %a, align 4
  ret void
}


declare double @llvm.maxnum.f64(double, double) #0
declare double @llvm.minnum.f64(double, double) #0

!igc.functions = !{!0}

!0 = !{ptr @test_customloop, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
