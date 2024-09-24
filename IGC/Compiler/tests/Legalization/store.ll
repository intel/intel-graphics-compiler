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

; Checks legalization of illegal int stores

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"


define void @test_store_i24(ptr addrspace(3) %sptr, i24 %src) {
; CHECK-LABEL: define void @test_store_i24(
; CHECK-SAME: ptr addrspace(3) [[SPTR:%.*]], i24 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i24 [[SRC]] to <3 x i8>
; CHECK:    [[TMP2:%.*]] = bitcast ptr addrspace(3) [[SPTR]] to ptr addrspace(3)
; CHECK:    store <3 x i8> [[TMP1]], ptr addrspace(3) [[TMP2]], align 4
; CHECK:    ret void
;
  store i24 %src, ptr addrspace(3) %sptr, align 4
  ret void
}

define ptr addrspace(3) @test_store_inttoptr(i96 %src) {
; CHECK-LABEL: define ptr addrspace(3) @test_store_inttoptr(
; CHECK-SAME: i96 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = inttoptr i32 144 to ptr addrspace(3)
; CHECK:    [[TMP2:%.*]] = bitcast i96 [[SRC]] to <3 x i32>
; CHECK:    [[TMP3:%.*]] = inttoptr i32 144 to ptr addrspace(3)
; CHECK:    store <3 x i32> [[TMP2]], ptr addrspace(3) [[TMP3]], align 8
; CHECK:    ret ptr addrspace(3) [[TMP1]]
;
  %1 = inttoptr i32 144 to ptr addrspace(3)
  store i96 %src, ptr addrspace(3) %1, align 8
  ret ptr addrspace(3) %1
}

define void @test_store_i16(ptr addrspace(3) %sptr, i16 %src) {
; CHECK-LABEL: define void @test_store_i16(
; CHECK-SAME: ptr addrspace(3) [[SPTR:%.*]], i16 [[SRC:%.*]]) {
; CHECK:    store i16 [[SRC]], ptr addrspace(3) [[SPTR]], align 2
; CHECK:    ret void
;
  store i16 %src, ptr addrspace(3) %sptr, align 2
  ret void
}

!igc.functions = !{!0, !2, !3}

!0 = !{ptr @test_store_i24, !1}
!1 = !{}
!2 = !{ptr @test_store_inttoptr, !1}
!3 = !{ptr @test_store_i16, !1}
