;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -platformbmg --igc-sink-ptr-const-add -S < %s | FileCheck %s
; ------------------------------------------------
; SinkPointerConstAdd
; ------------------------------------------------
;
; Sign must flip when sinking a constant out of a Sub's subtrahend, one Add
; level deep: base - (x + 128) folds to base - x - 128.

declare i64 @mock.get.i64()
declare void @mock.consume.ptr(i32 addrspace(1)*)

define void @main(<8 x i32> %r0, i8* %privateBase) #0 {
entry:
  %base = call i64 @mock.get.i64()
  %x = call i64 @mock.get.i64()
  %xPlusC = add i64 %x, 128
  %c2 = sub i64 %base, %xPlusC
  %ptr = inttoptr i64 %c2 to i32 addrspace(1)*
  call void @mock.consume.ptr(i32 addrspace(1)* %ptr)
; CHECK: %[[BASE:[0-9a-zA-Z_.]+]] = call i64 @mock.get.i64()
; CHECK-NEXT: %[[X:[0-9a-zA-Z_.]+]] = call i64 @mock.get.i64()
; CHECK-NEXT: %[[SUB:[0-9a-zA-Z_.]+]] = sub i64 %[[BASE]], %[[X]]
; CHECK-NEXT: %[[SUNK:[0-9a-zA-Z_.]+]] = add i64 %[[SUB]], -128
; CHECK-NEXT: %[[PTR:[0-9a-zA-Z_.]+]] = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
; CHECK-NEXT: call void @mock.consume.ptr(ptr addrspace(1) %[[PTR]])
  ret void
}

attributes #0 = { alwaysinline null_pointer_is_valid }

!igc.functions = !{!0}
!0 = !{void (<8 x i32>, i8*)* @main, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
