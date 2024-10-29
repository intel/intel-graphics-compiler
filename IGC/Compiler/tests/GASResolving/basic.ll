;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-gas-resolve --opaque-pointers -S < %s | FileCheck %s
; REQUIRES: llvm-14-plus, opaque-ptr-fix


;the pass should remove addrspacecast to addrspace(4) and replace uses with origin type

define i32 @basic(ptr addrspace(1) %src) {
; CHECK-LABEL: define i32 @basic(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = load i32, ptr addrspace(1) [[SRC]], align 4
; CHECK-NEXT:    ret i32 [[TMP1]]
;
  %1 = addrspacecast ptr addrspace(1) %src to ptr addrspace(4)
  %2 = load i32, ptr addrspace(4) %1

  ret i32 %2
}


define i32 @should_omit_non_GAS_pointer(ptr addrspace(1) %src) {
  %1 = addrspacecast ptr addrspace(1) %src to ptr addrspace(2)
  %2 = load i32, ptr addrspace(2) %1

  ret i32 %2
}

define void @should_remove_when_not_used(ptr addrspace(1) %src) {
  %1 = addrspacecast ptr addrspace(1) %src to ptr addrspace(4)

  ret void
}

define ptr addrspace(4) @should_add_addrspacecast(ptr addrspace(1) %src) {
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src
  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_not_add_addrspacecast(ptr addrspace(1) %src) {
  %1 = inttoptr i64 0 to ptr addrspace(4)
  store ptr addrspace(4) %1, ptr addrspace(1) %src
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

%struct.with.pointer = type { ptr addrspace(4), i32 }

define ptr addrspace(4) @should_add_addrspacecast_with_struct(ptr addrspace(1) %src) {
  %pointer = getelementptr %struct.with.pointer, ptr addrspace(1) %src, i32 0, i32 0
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %pointer

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_add_addrspacecast_with_struct_2(ptr addrspace(1) %src) {
  %1 = inttoptr i32 13 to ptr addrspace(2)
  %2 = addrspacecast ptr addrspace(2) %1 to ptr addrspace(4)
  %3 = getelementptr %struct.with.pointer, ptr addrspace(1) %src, i32 0, i32 0
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %3
  %pointer_value2 = addrspacecast ptr addrspace(4) %pointer_value to ptr addrspace(2)
  store i32 13, ptr addrspace(4) %pointer_value
  store i32 14, ptr addrspace(2) %pointer_value2
  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_add_addrspacecast_since_store_uses_different_pointer(ptr addrspace(1) %src) {
  %1 = alloca ptr addrspace(4), addrspace(1)
  %2 = inttoptr i64 0 to ptr addrspace(4)
  store ptr addrspace(4) %2, ptr addrspace(1) %1
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_add_addrspacecast_since_store_uses_different_pointer_2(ptr addrspace(1) %src) {
entry:
  %0 = alloca ptr addrspace(4), addrspace(1)
  %1 = inttoptr i64 0 to ptr addrspace(4)
  br label %bb1
bb1:
  store ptr addrspace(4) %1, ptr addrspace(1) %0
  br label %bb2
bb2:
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_not_add_addrspacecast_when_call_modifies_pointer(ptr addrspace(1) %src) {
  %1 = inttoptr i64 0 to ptr addrspace(1)
  call void @llvm.memcpy.p0.p0.i32(ptr addrspace(1) %src, ptr addrspace(1) %1, i32 0, i1 0)
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}


define ptr addrspace(4) @should_not_add_addrspacecast_when_instruction_may_modify_pointer(ptr addrspace(1) %src) {
  %1 = inttoptr i64 0 to ptr addrspace(4)
  fence acquire
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(1) @edge_case_check(ptr addrspace(1) %src) {
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src
  %1 = addrspacecast ptr addrspace(4) %pointer_value to ptr addrspace(2)
  %2 = addrspacecast ptr addrspace(2) %1 to ptr addrspace(1)
  ret ptr addrspace(1) %2
}

declare void @llvm.memcpy.p0.p0.i32(ptr addrspace(1), ptr addrspace(1), i32 , i1)

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8, !9, !10, !11}
!0 = !{ptr addrspace(4) (ptr addrspace(1))* @should_not_add_addrspacecast, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr addrspace(4) (ptr addrspace(1))* @should_add_addrspacecast, !1}
!4 = !{ptr addrspace(4) (ptr addrspace(1))* @should_add_addrspacecast_with_struct, !1}
!5 = !{ptr addrspace(4) (ptr addrspace(1))* @should_add_addrspacecast_since_store_uses_different_pointer, !1}
!6 = !{ptr addrspace(4) (ptr addrspace(1))* @should_not_add_addrspacecast_when_call_modifies_pointer, !1}
!7 = !{ptr addrspace(4) (ptr addrspace(1))* @should_not_add_addrspacecast_when_instruction_may_modify_pointer, !1}
!8 = !{ptr addrspace(4) (ptr addrspace(1))* @should_not_add_addrspacecast_2, !1}
!9 = !{ptr addrspace(4) (ptr addrspace(1))* @should_add_addrspacecast_since_store_uses_different_pointer_2, !1}
!10 = !{ptr addrspace(1) (ptr addrspace(1))* @edge_case_check, !1}
!11 = !{ptr addrspace(4) (ptr addrspace(1))* @should_add_addrspacecast_with_struct_2, !1}
