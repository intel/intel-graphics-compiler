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
; Legalization: store
; ------------------------------------------------

; Checks legalization of i1 store

define void @test_store_i1(ptr addrspace(3) %sptr, i1 %src) {
; CHECK-LABEL: define void @test_store_i1(
; CHECK-SAME: ptr addrspace(3) [[SPTR:%.*]], i1 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = zext i1 [[SRC]] to i8
; CHECK:    store i8 [[TMP1]], ptr addrspace(3) [[SPTR]], align 1
; CHECK:    ret void
;
  store i1 %src, ptr addrspace(3) %sptr, align 1
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test_store_i1, !1}
!1 = !{}
