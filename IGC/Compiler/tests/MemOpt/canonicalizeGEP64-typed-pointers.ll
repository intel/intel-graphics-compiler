;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-memopt -instcombine | FileCheck %s

define void @f0(i32 %arg1, i32 %arg2, double addrspace(1)* %src, double addrspace(1)* %dst) {
entry:
  %or1 = or i32 %arg1, 1
  %add1 = add nsw i32 %or1, %arg2
  %sext1 = sext i32 %add1 to i64
  %add2 = add nsw i64 %sext1, -1
  %gep1 = getelementptr inbounds double, double addrspace(1)* %src, i64 %add2
  %load1 = load double, double addrspace(1)* %gep1, align 8
  %gep2 = getelementptr inbounds double, double addrspace(1)* %src, i64 %sext1
  %load2 = load double, double addrspace(1)* %gep2, align 8
  ; Check that load instructions were merged
  ;
  ; CHECK:    {{.*}} = load <2 x double>, <2 x double> addrspace(1)* {{.*}}, align
  %add3 = fadd fast double %load1, %load2
  store double %add3, double addrspace(1)* %dst, align 8
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32, i32, double addrspace(1)*, double addrspace(1)*)* @f0, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
