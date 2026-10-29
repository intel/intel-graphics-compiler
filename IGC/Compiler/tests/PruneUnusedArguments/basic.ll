;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -enable-debugify -PruneUnusedArguments -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -enable-debugify -PruneUnusedArguments -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}
; ------------------------------------------------
; PruneUnusedArguments
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @foo(i32* %b, i32 %a) {
; CHECK-LABEL: define {{.*}} @foo(
; CHECK:  entry:
; CHECK-TYPED:    [[TMP0:%.*]] = load i32, i32* [[B:%[A-z0-9]*]]
; CHECK-OPAQUE:    [[TMP0:%.*]] = load i32, ptr [[B:%[A-z0-9]*]]
; CHECK:    [[TMP1:%.*]] = add i32 [[TMP0]], 13
; CHECK-TYPED:    store i32 [[TMP1]], i32* [[B]]
; CHECK-OPAQUE:    store i32 [[TMP1]], ptr [[B]]
; CHECK:    ret void
;
entry:
  %0 = load i32, i32* %b
  %1 = add i32 %0, 13
  store i32 %1, i32* %b
  ret void
}

define spir_kernel void @test(i32* %src1, i32 %src2) {
; CHECK-LABEL: @test(
; CHECK-TYPED:    call spir_func void @foo(i32* [[SRC1:%.*]], i32 undef)
; CHECK-OPAQUE:    call spir_func void @foo(ptr [[SRC1:%.*]], i32 undef)
; CHECK:    ret void
;
  call spir_func void @foo(i32* %src1, i32 %src2)
  ret void
}
