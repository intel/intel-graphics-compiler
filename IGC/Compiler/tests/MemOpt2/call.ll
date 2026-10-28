;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-memopt2 -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-memopt2 -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}
; ------------------------------------------------
; MemOpt2
; ------------------------------------------------

define void @test(i32 addrspace(1)* %addr) {
; CHECK-LABEL: @test(
; CHECK-TYPED: store i32 2, i32* %alloca, align 4
; CHECK-OPAQUE: store i32 2, ptr %alloca, align 4
; CHECK-TYPED-NEXT: call i32 addrspace(1)* @callee(i32 addrspace(1)* %addr, i32* %alloca)
; CHECK-OPAQUE-NEXT: call ptr addrspace(1) @callee(ptr addrspace(1) %addr, ptr %alloca)

entry:
  %alloca = alloca i32
  %0 = getelementptr i32, i32 addrspace(1)* %addr, i32 0
  %1 = load i32, i32 addrspace(1)* %0, align 4
  %2 = getelementptr i32, i32 addrspace(1)* %addr, i32 1
  %3 = load i32, i32 addrspace(1)* %2, align 4
  %4 = add i32 %3, 32
  store i32 2, i32* %alloca, align 4
  %addr2 = call i32 addrspace(1)* @callee(i32 addrspace(1)* %addr, i32* %alloca)
  %5 = load i32, i32 addrspace(1)* %addr2, align 4
  %6 = add i32 %5, 64
  store i32 %1, i32 addrspace(1)* %addr, align 4
  store i32 %4, i32 addrspace(1)* %addr, align 4
  store i32 %6, i32 addrspace(1)* %addr, align 4
  ret void
}

define i32 addrspace(1)* @callee(i32 addrspace(1)* %addr, i32* %offset) {
  %1 = load i32, i32* %offset
  %2 = getelementptr i32, i32 addrspace(1)* %addr, i32 %1
  ret i32 addrspace(1)* %2
}

!igc.functions = !{!0, !1}

!0 = !{void (i32 addrspace(1)*)* @test, !2}
!1 = !{i32 addrspace(1)* (i32 addrspace(1)*, i32*)* @callee, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
