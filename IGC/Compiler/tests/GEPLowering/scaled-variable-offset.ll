;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -platformpvc -igc-gep-lowering -S < %s | FileCheck %s

define spir_kernel void @negative_local(i32 %index) {
; CHECK-LABEL: @negative_local(
; CHECK: %[[SCALED:.*]] = shl i32 %index, 2
; CHECK-NEXT: %[[BASE:.*]] = add i32 0, %[[SCALED]]
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], -32
; CHECK: inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %offset = add i32 %index, -8
  %extended = zext i32 %offset to i64
  %address = getelementptr i32, ptr addrspace(3) null, i64 %extended
  ret void
}

define spir_kernel void @positive_wrap(i32 %index) {
; CHECK-LABEL: @positive_wrap(
; CHECK: %[[SCALED:.*]] = shl i32 %index, 2
; CHECK-NEXT: %[[BASE:.*]] = add i32 0, %[[SCALED]]
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], 4
; CHECK: inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %offset = add i32 %index, 1073741825
  %address = getelementptr i32, ptr addrspace(3) null, i32 %offset
  ret void
}

define spir_kernel void @negative_wrap(i32 %index) {
; CHECK-LABEL: @negative_wrap(
; CHECK: %[[SCALED:.*]] = shl i32 %index, 2
; CHECK-NEXT: %[[BASE:.*]] = add i32 0, %[[SCALED]]
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], -4
; CHECK: inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %offset = add i32 %index, -1073741825
  %address = getelementptr i32, ptr addrspace(3) null, i32 %offset
  ret void
}

define spir_kernel void @wrap_to_zero(i32 %index) {
; CHECK-LABEL: @wrap_to_zero(
; CHECK: %[[SCALED:.*]] = shl i32 %index, 2
; CHECK-NEXT: %[[BASE:.*]] = add i32 0, %[[SCALED]]
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], 0
; CHECK-NEXT: %{{.*}} = inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %offset = add i32 %index, 1073741824
  %address = getelementptr i32, ptr addrspace(3) null, i32 %offset
  ret void
}

define spir_kernel void @negative_global(i32 %index) {
; CHECK-LABEL: @negative_global(
; CHECK: %[[EXTENDED:.*]] = sext i32 %index to i64
; CHECK: %[[SCALED:.*]] = shl i64 %[[EXTENDED]], 2
; CHECK-NEXT: %[[BASE:.*]] = add i64 0, %[[SCALED]]
; CHECK-NEXT: %[[OFFSET:.*]] = add i64 %[[BASE]], -32
; CHECK: inttoptr i64 %[[OFFSET]] to ptr addrspace(1)
  %offset = add nsw i32 %index, -8
  %extended = sext i32 %offset to i64
  %address = getelementptr i32, ptr addrspace(1) null, i64 %extended
  ret void
}

!igc.functions = !{!0, !1, !2, !3, !4}
!0 = !{ptr @negative_local, !5}
!1 = !{ptr @positive_wrap, !5}
!2 = !{ptr @negative_wrap, !5}
!3 = !{ptr @wrap_to_zero, !5}
!4 = !{ptr @negative_global, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
