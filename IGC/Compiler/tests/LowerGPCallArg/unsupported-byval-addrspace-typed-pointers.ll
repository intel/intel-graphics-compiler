;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-lower-gp-arg -S < %s 2>&1 | FileCheck %s

@a = internal addrspace(3) global i32 0
; CHECK: [[A:@.*]] = internal addrspace(3) global i32 0

define i32 @func(i32 addrspace(4)* byval (i32) %p1) {
  ; CHECK: define i32 [[FUNC:@.*]](i32 addrspace(4)* byval(i32) [[P1:%.*]]) {
  %1 = load i32, i32 addrspace(4)* %p1
  ret i32 %1
}

define void @kernel() {
  ; CHECK: [[TMP1:%.*]] = addrspacecast i32 addrspace(3)* [[A]] to i32 addrspace(4)*
  %1 = addrspacecast i32 addrspace(3)* @a to i32 addrspace(4)*
  call i32 @func(i32 addrspace(4)* byval(i32) %1)
  ret void
}

!igc.functions = !{!0, !3}

!0 = !{void ()* @kernel, !1}
!3 = !{i32 (i32 addrspace(4)*)* @func, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
