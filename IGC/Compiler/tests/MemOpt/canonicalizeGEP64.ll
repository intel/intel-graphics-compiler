;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-memopt -instcombine | FileCheck %s

define void @f0(i32 %arg1, i32 %arg2, ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %or1 = or i32 %arg1, 1
  %add1 = add nsw i32 %or1, %arg2
  %sext1 = sext i32 %add1 to i64
  %add2 = add nsw i64 %sext1, -1
  %gep1 = getelementptr inbounds double, ptr addrspace(1) %src, i64 %add2
  %load1 = load double, ptr addrspace(1) %gep1, align 8
  %gep2 = getelementptr inbounds double, ptr addrspace(1) %src, i64 %sext1
  %load2 = load double, ptr addrspace(1) %gep2, align 8
  ; Check that load instructions were merged
  ;
  ; CHECK:    {{.*}} = load <2 x double>, ptr addrspace(1) {{.*}}, align
  %add3 = fadd fast double %load1, %load2
  store double %add3, ptr addrspace(1) %dst, align 8
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @f0, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
