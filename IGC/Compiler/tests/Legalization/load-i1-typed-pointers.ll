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
; Legalization: load
; ------------------------------------------------

; Checks legalization of i1 load

define i1 @test_load_i1(i1 addrspace(3)* %sptr) {
; CHECK-LABEL: define i1 @test_load_i1(
; CHECK-SAME: i1 addrspace(3)* [[SPTR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i1 addrspace(3)* [[SPTR]] to i8 addrspace(3)*
; CHECK:    [[TMP2:%.*]] = load i8, i8 addrspace(3)* [[TMP1]], align 1
; CHECK:    [[TMP3:%.*]] = trunc i8 [[TMP2]] to i1
; CHECK:    ret i1 [[TMP3]]
;
  %1 = load i1, i1 addrspace(3)* %sptr, align 1
  ret i1 %1
}

!igc.functions = !{!0}

!0 = !{i1 (i1 addrspace(3)*)* @test_load_i1, !1}
!1 = !{}
