;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify --igc-gen-specific-pattern -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: cmp pattern
; ------------------------------------------------

; Debug-info related check
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 3
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define i32 @test1(i32 %src) {
; CHECK-LABEL: define i32 @test1(
; CHECK:    ret i32 %src
  %i2p = inttoptr i32 %src to ptr addrspace(1)
  %p2i = ptrtoint ptr addrspace(1) %i2p to i64
  %res = trunc i64 %p2i to i32
  ret i32 %res
}

define i32 @test2(i32 %src) {
; CHECK-LABEL: define i32 @test2(
; CHECK: [[RES:%.*]] = add i32 %src, 17
; CHECK: ret i32 [[RES]]
 %i2p = inttoptr i32 %src to ptr addrspace(1)
 %p2i = ptrtoint ptr addrspace(1) %i2p to i64
 %add = add i64 %p2i, 17
 %res = trunc i64 %add to i32
 ret i32 %res
}

define i32 @test3(i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test3(
; CHECK: [[RES:%.*]] = add i32 %a, %b
; CHECK: ret i32 [[RES]]
  %s1 = sext i32 %a to i64
  %s2 = sext i32 %b to i64
  %add = add i64 %s1, %s2
  %res = trunc i64 %add to i32
  ret i32 %res
}
