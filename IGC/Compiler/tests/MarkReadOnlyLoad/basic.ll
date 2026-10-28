;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -debugify -mark-readonly-load -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -debugify -mark-readonly-load -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}
; ------------------------------------------------
; MarkReadOnlyLoad
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(i32 addrspace(2)* %src1, i32* %dst) {
; CHECK-LABEL: @test(
; CHECK-TYPED:    [[TMP1:%.*]] = load i32, i32 addrspace(2)* %src1,{{.*}} !invariant.load [[NULL_MD:![0-9]*]]
; CHECK-OPAQUE:    [[TMP1:%.*]] = load i32, ptr addrspace(2) %src1,{{.*}} !invariant.load [[NULL_MD:![0-9]*]]
; CHECK-TYPED:    store i32 [[TMP1]], i32* %dst
; CHECK-OPAQUE:    store i32 [[TMP1]], ptr %dst
; CHECK:    ret void
;
  %1 = load i32, i32 addrspace(2)* %src1
  store i32 %1, i32* %dst
  ret void
}

; CHECK: [[NULL_MD]] = !{null}
