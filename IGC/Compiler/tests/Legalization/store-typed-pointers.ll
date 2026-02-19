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

; Checks legalization of illegal int stores

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define void @test_store_i24(i24 addrspace(3)* %sptr, i24 %src) {
; CHECK-LABEL: define void @test_store_i24(
; CHECK-SAME: i24 addrspace(3)* [[SPTR:%.*]], i24 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i24 [[SRC]] to <3 x i8>
; CHECK:    [[TMP2:%.*]] = bitcast i24 addrspace(3)* [[SPTR]] to <3 x i8> addrspace(3)*
; CHECK:    store <3 x i8> [[TMP1]], <3 x i8> addrspace(3)* [[TMP2]]
; CHECK:    ret void
;
  store i24 %src, i24 addrspace(3)* %sptr
  ret void
}

define i96 addrspace(3)* @test_store_inttoptr(i96 %src) {
; CHECK-LABEL: define i96 addrspace(3)* @test_store_inttoptr(
; CHECK-SAME: i96 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = inttoptr i32 144 to i96 addrspace(3)*
; CHECK:    [[TMP2:%.*]] = bitcast i96 [[SRC]] to <3 x i32>
; CHECK:    [[TMP3:%.*]] = inttoptr i32 144 to <3 x i32> addrspace(3)*
; CHECK:    store <3 x i32> [[TMP2]], <3 x i32> addrspace(3)* [[TMP3]]
; CHECK:    ret i96 addrspace(3)* [[TMP1]]
;
  %1 = inttoptr i32 144 to i96 addrspace(3)*
  store i96 %src, i96 addrspace(3)* %1
  ret i96 addrspace(3)* %1
}

; Sanity(no legalization)

define void @test_store_i16(i16 addrspace(3)* %sptr, i16 %src) {
; CHECK-LABEL: define void @test_store_i16(
; CHECK-SAME: i16 addrspace(3)* [[SPTR:%.*]], i16 [[SRC:%.*]]) {
; CHECK:    store i16 [[SRC]], i16 addrspace(3)* [[SPTR]]
; CHECK:    ret void
;
  store i16 %src, i16 addrspace(3)* %sptr
  ret void
}

!igc.functions = !{!0, !1, !2}
!0 = !{void (i24 addrspace(3)*, i24)* @test_store_i24, !3}
!1 = !{i96 addrspace(3)* (i96)* @test_store_inttoptr, !3}
!2 = !{void (i16 addrspace(3)*, i16)* @test_store_i16, !3}
!3 = !{}
