;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -prepare-loads-stores -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PrepareLoadsStoresPass
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_load_i64(i64 addrspace(1)* %a) {
; CHECK-LABEL: @test_load_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 addrspace(1)* %a to <2 x i32> addrspace(1)*
; CHECK:    [[TMP2:%.*]] = load <2 x i32>, <2 x i32> addrspace(1)* [[TMP1]]
; CHECK:    [[TMP3:%.*]] = bitcast <2 x i32> [[TMP2]] to i64
; CHECK:    call void @use.i64(i64 [[TMP3]])
; CHECK:    ret void
;
  %1 = load i64, i64 addrspace(1)* %a
  call void @use.i64(i64 %1)
  ret void
}

define void @test_load_v2i64(<2 x i64> addrspace(1)* %a) {
; CHECK-LABEL: @test_load_v2i64(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i64> addrspace(1)* %a to <4 x i32> addrspace(1)*
; CHECK:    [[TMP2:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[TMP1]]
; CHECK:    [[TMP3:%.*]] = bitcast <4 x i32> [[TMP2]] to <2 x i64>
; CHECK:    call void @use.v2i64(<2 x i64> [[TMP3]])
; CHECK:    ret void
;
  %1 = load <2 x i64>, <2 x i64> addrspace(1)* %a
  call void @use.v2i64(<2 x i64> %1)
  ret void
}

declare void @use.i64(i64)
declare void @use.v2i64(<2 x i64>)


define void @test_store_i64(i64 addrspace(1)* %a, i64 %b) {
; CHECK-LABEL: @test_store_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %b to <2 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast i64 addrspace(1)* %a to <2 x i32> addrspace(1)*
; CHECK:    store <2 x i32> [[TMP1]], <2 x i32> addrspace(1)* [[TMP2]]
; CHECK:    ret void
;
  store i64 %b, i64 addrspace(1)* %a
  ret void
}

define void @test_store_v2i64(<2 x i64> addrspace(1)* %a, <2 x i64> %b) {
; CHECK-LABEL: @test_store_v2i64(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i64> %b to <4 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast <2 x i64> addrspace(1)* %a to <4 x i32> addrspace(1)*
; CHECK:    store <4 x i32> [[TMP1]], <4 x i32> addrspace(1)* [[TMP2]]
; CHECK:    ret void
;
  store <2 x i64> %b, <2 x i64> addrspace(1)* %a
  ret void
}

