;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -enable-debugify -SimplifyConstant -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SimplifyConstant
; ------------------------------------------------

; Debug-info related check
;
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 1
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

@a = private addrspace(2) constant [4 x i32] [i32 13, i32 42, i32 13, i32 42], align 1

define spir_kernel void @test_simpleconst(i32 %a, i32* %b) {
; CHECK-LABEL: @test_simpleconst(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = trunc i32 [[A:%.*]] to i1
; CHECK:    [[TMP1:%.*]] = select i1 [[TMP0]], i32 42, i32 13
; CHECK:    store i32 [[TMP1]], i32* [[B:%.*]], align 4
; CHECK:    ret void
;
entry:
  %0 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(2)* @a, i32 0, i32 %a
  %1 = load i32, i32 addrspace(2)* %0, align 4
  store i32 %1, i32* %b, align 4
  ret void
}
