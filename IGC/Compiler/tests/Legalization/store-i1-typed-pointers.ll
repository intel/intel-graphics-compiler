;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: store
; ------------------------------------------------

; Checks legalization of i1 store

define void @test_store_i1(i1 addrspace(3)* %sptr, i1 %src) {
; CHECK-LABEL: define void @test_store_i1(
; CHECK-SAME: i1 addrspace(3)* [[SPTR:%.*]], i1 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = zext i1 [[SRC]] to i8
; CHECK:    [[TMP2:%.*]] = bitcast i1 addrspace(3)* [[SPTR]] to i8 addrspace(3)*
; CHECK:    store i8 [[TMP1]], i8 addrspace(3)* [[TMP2]]
; CHECK:    ret void
;
  store i1 %src, i1 addrspace(3)* %sptr
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i1 addrspace(3)*, i1)* @test_store_i1, !1}
!1 = !{}
