;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -opaque-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: load
; ------------------------------------------------

; Checks legalization of i1 load

define i1 @test_load_i1(ptr addrspace(3) %sptr) {
; CHECK-LABEL: define i1 @test_load_i1(
; CHECK-SAME: ptr addrspace(3) [[SPTR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = load i8, ptr addrspace(3) [[SPTR]], align 1
; CHECK:    [[TMP2:%.*]] = trunc i8 [[TMP1]] to i1
; CHECK:    ret i1 [[TMP2]]
;
  %1 = load i1, ptr addrspace(3) %sptr, align 1
  ret i1 %1
}

!igc.functions = !{!0}

!0 = !{ptr @test_load_i1, !1}
!1 = !{}
