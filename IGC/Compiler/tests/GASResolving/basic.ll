;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-gas-resolve --opaque-pointers -S < %s | FileCheck %s


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
; CHECK-LABEL: define i32 @should_omit_non_GAS_pointer(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = addrspacecast ptr addrspace(1) [[SRC]] to ptr addrspace(2)
; CHECK-NEXT:    [[TMP2:%.*]] = load i32, ptr addrspace(2) [[TMP1]], align 4
; CHECK-NEXT:    ret i32 [[TMP2]]
;
  %1 = addrspacecast ptr addrspace(1) %src to ptr addrspace(2)
  %2 = load i32, ptr addrspace(2) %1

  ret i32 %2
}

define void @should_remove_when_not_used(ptr addrspace(1) %src) {
; CHECK-LABEL: define void @should_remove_when_not_used(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    ret void
;
  %1 = addrspacecast ptr addrspace(1) %src to ptr addrspace(4)

  ret void
}

define ptr addrspace(4) @should_add_addrspacecast(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(4) @should_add_addrspacecast(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    [[TMP1:%.*]] = addrspacecast ptr addrspace(4) [[POINTER_VALUE]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP2:%.*]] = addrspacecast ptr addrspace(1) [[TMP1]] to ptr addrspace(4)
; CHECK-NEXT:    ret ptr addrspace(4) [[TMP2]]
;
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src
  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_not_add_addrspacecast(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(4) @should_not_add_addrspacecast(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = inttoptr i64 0 to ptr addrspace(4)
; CHECK-NEXT:    store ptr addrspace(4) [[TMP1]], ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    ret ptr addrspace(4) [[POINTER_VALUE]]
;
  %1 = inttoptr i64 0 to ptr addrspace(4)
  store ptr addrspace(4) %1, ptr addrspace(1) %src
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_not_add_addrspacecast_2(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(4) @should_not_add_addrspacecast_2(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP0:%.*]] = inttoptr i64 0 to ptr addrspace(4)
; CHECK-NEXT:    br label %[[BB1:.*]]
; CHECK:       [[BB1]]:
; CHECK-NEXT:    store ptr addrspace(4) [[TMP0]], ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    br label %[[BB2:.*]]
; CHECK:       [[BB2]]:
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    ret ptr addrspace(4) [[POINTER_VALUE]]
;
entry:
  %0 = inttoptr i64 0 to ptr addrspace(4)
  br label %bb1
bb1:
  store ptr addrspace(4) %0, ptr addrspace(1) %src
  br label %bb2
bb2:
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

%struct.with.pointer = type { ptr addrspace(4), i32 }

define ptr addrspace(4) @should_add_addrspacecast_with_struct(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(4) @should_add_addrspacecast_with_struct(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[POINTER:%.*]] = getelementptr [[STRUCT_WITH_POINTER:%.*]], ptr addrspace(1) [[SRC]], i32 0, i32 0
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[POINTER]], align 8
; CHECK-NEXT:    [[TMP1:%.*]] = addrspacecast ptr addrspace(4) [[POINTER_VALUE]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP2:%.*]] = addrspacecast ptr addrspace(1) [[TMP1]] to ptr addrspace(4)
; CHECK-NEXT:    ret ptr addrspace(4) [[TMP2]]
;
  %pointer = getelementptr %struct.with.pointer, ptr addrspace(1) %src, i32 0, i32 0
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %pointer

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_add_addrspacecast_with_struct_2(ptr addrspace(1) %src) {
; CHECK-LABEL: @should_add_addrspacecast_with_struct_2(
; CHECK-NEXT:    [[TMP1:%.*]] = inttoptr i32 13 to ptr addrspace(2)
; CHECK-NEXT:    [[TMP2:%.*]] = getelementptr [[ST:%.*]], ptr addrspace(1) [[SRC:%.*]], i32 0, i32 0
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[TMP2]], align 8
; CHECK-NEXT:    [[TMP3:%.*]] = addrspacecast ptr addrspace(4) [[POINTER_VALUE]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP4:%.*]] = addrspacecast ptr addrspace(1) [[TMP3]] to ptr addrspace(4)
; CHECK-NEXT:    [[POINTER_VALUE2:%.*]] = addrspacecast ptr addrspace(4) [[TMP4]] to ptr addrspace(2)
; CHECK-NEXT:    store i32 13, ptr addrspace(1) [[TMP3]], align 4
; CHECK-NEXT:    store i32 14, ptr addrspace(2) [[POINTER_VALUE2]], align 4
; CHECK-NEXT:    ret ptr addrspace(4) [[TMP4]]
;
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
; CHECK-LABEL: define ptr addrspace(4) @should_add_addrspacecast_since_store_uses_different_pointer(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = alloca ptr addrspace(4), align 8, addrspace(1)
; CHECK-NEXT:    [[TMP2:%.*]] = inttoptr i64 0 to ptr addrspace(4)
; CHECK-NEXT:    store ptr addrspace(4) [[TMP2]], ptr addrspace(1) [[TMP1]], align 8
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    [[TMP3:%.*]] = addrspacecast ptr addrspace(4) [[POINTER_VALUE]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP4:%.*]] = addrspacecast ptr addrspace(1) [[TMP3]] to ptr addrspace(4)
; CHECK-NEXT:    ret ptr addrspace(4) [[TMP4]]
;
  %1 = alloca ptr addrspace(4), addrspace(1)
  %2 = inttoptr i64 0 to ptr addrspace(4)
  store ptr addrspace(4) %2, ptr addrspace(1) %1
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(4) @should_add_addrspacecast_since_store_uses_different_pointer_2(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(4) @should_add_addrspacecast_since_store_uses_different_pointer_2(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP0:%.*]] = alloca ptr addrspace(4), align 8, addrspace(1)
; CHECK-NEXT:    [[TMP1:%.*]] = inttoptr i64 0 to ptr addrspace(4)
; CHECK-NEXT:    br label %[[BB1:.*]]
; CHECK:       [[BB1]]:
; CHECK-NEXT:    store ptr addrspace(4) [[TMP1]], ptr addrspace(1) [[TMP0]], align 8
; CHECK-NEXT:    br label %[[BB2:.*]]
; CHECK:       [[BB2]]:
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    [[TMP2:%.*]] = addrspacecast ptr addrspace(4) [[POINTER_VALUE]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP3:%.*]] = addrspacecast ptr addrspace(1) [[TMP2]] to ptr addrspace(4)
; CHECK-NEXT:    ret ptr addrspace(4) [[TMP3]]
;
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
; CHECK-LABEL: define ptr addrspace(4) @should_not_add_addrspacecast_when_call_modifies_pointer(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = inttoptr i64 0 to ptr addrspace(1)
; CHECK-NEXT:    call void @llvm.memcpy.p1.p1.i32(ptr addrspace(1) [[SRC]], ptr addrspace(1) [[TMP1]], i32 0, i1 false)
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    ret ptr addrspace(4) [[POINTER_VALUE]]
;
  %1 = inttoptr i64 0 to ptr addrspace(1)
  call void @llvm.memcpy.p0.p0.i32(ptr addrspace(1) %src, ptr addrspace(1) %1, i32 0, i1 0)
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}


define ptr addrspace(4) @should_not_add_addrspacecast_when_instruction_may_modify_pointer(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(4) @should_not_add_addrspacecast_when_instruction_may_modify_pointer(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = inttoptr i64 0 to ptr addrspace(4)
; CHECK-NEXT:    fence acquire
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    ret ptr addrspace(4) [[POINTER_VALUE]]
;
  %1 = inttoptr i64 0 to ptr addrspace(4)
  fence acquire
  %pointer_value = load ptr addrspace(4), ptr addrspace(1) %src

  ret ptr addrspace(4) %pointer_value
}

define ptr addrspace(1) @edge_case_check(ptr addrspace(1) %src) {
; CHECK-LABEL: define ptr addrspace(1) @edge_case_check(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]]) {
; CHECK-NEXT:    [[POINTER_VALUE:%.*]] = load ptr addrspace(4), ptr addrspace(1) [[SRC]], align 8
; CHECK-NEXT:    [[TMP1:%.*]] = addrspacecast ptr addrspace(4) [[POINTER_VALUE]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP2:%.*]] = addrspacecast ptr addrspace(1) [[TMP1]] to ptr addrspace(4)
; CHECK-NEXT:    [[TMP3:%.*]] = addrspacecast ptr addrspace(4) [[TMP2]] to ptr addrspace(2)
; CHECK-NEXT:    [[TMP4:%.*]] = addrspacecast ptr addrspace(2) [[TMP3]] to ptr addrspace(1)
; CHECK-NEXT:    ret ptr addrspace(1) [[TMP4]]
;
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
