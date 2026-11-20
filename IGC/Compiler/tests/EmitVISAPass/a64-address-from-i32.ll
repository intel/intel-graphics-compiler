;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s

; An i32 address must not be split into an A64 base and offset: inttoptr zero-extends it after the add.

; CHECK-LABEL: .kernel "imm_offset"
; CHECK: lsc_store.ugm {{.*}} flat[{{[A-Za-z0-9_]+}}]:a64
define spir_kernel void @imm_offset(i32 %n) {
entry:
  %addr = add i32 %n, 4
  %ptr = inttoptr i32 %addr to ptr addrspace(1)
  store i32 0, ptr addrspace(1) %ptr, align 4
  ret void
}

; CHECK-LABEL: .kernel "uniform_base"
; CHECK: lsc_store.ugm {{.*}} flat[{{[A-Za-z0-9_]+}}]:a64
define spir_kernel void @uniform_base(i32 %n, i32 %m) {
entry:
  %addr = add i32 %n, %m
  %ptr = inttoptr i32 %addr to ptr addrspace(1)
  store i32 0, ptr addrspace(1) %ptr, align 4
  ret void
}

!igc.functions = !{!0, !1}
!0 = !{ptr @imm_offset, !2}
!1 = !{ptr @uniform_base, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
