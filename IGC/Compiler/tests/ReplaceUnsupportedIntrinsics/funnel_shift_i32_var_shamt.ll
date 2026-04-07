;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

; ============================================================
; i32 fshl/fshr - variable shamt
; ============================================================

define i32 @A1(i32, i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and i32 %2, 31
; CHECK:  [[CMP:%[a-zA-Z0-9]+]] = icmp eq i32 [[AND]], 0
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub i32 32, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, [[AND]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, [[SUB]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 [[CMP]], i32 %0, i32 [[OR]]
; CHECK:  ret i32 [[SEL]]
  %3 = call i32 @llvm.fshl.i32(i32 %0, i32 %1, i32 %2)
  ret i32 %3
}

define i32 @B1(i32, i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and i32 %2, 31
; CHECK:  [[CMP:%[a-zA-Z0-9]+]] = icmp eq i32 [[AND]], 0
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub i32 32, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, [[SUB]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, [[AND]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 [[CMP]], i32 %1, i32 [[OR]]
; CHECK:  ret i32 [[SEL]]
  %3 = call i32 @llvm.fshr.i32(i32 %0, i32 %1, i32 %2)
  ret i32 %3
}

; ============================================================
; i32 fshl/fshr - constant shamt=0
; ============================================================

define i32 @A2(i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 0
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, 32
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 true, i32 %0, i32 [[OR]]
; CHECK:  ret i32 [[SEL]]
  %2 = call i32 @llvm.fshl.i32(i32 %0, i32 %1, i32 0)
  ret i32 %2
}

define i32 @B2(i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 32
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, 0
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 true, i32 %1, i32 [[OR]]
; CHECK:  ret i32 [[SEL]]
  %2 = call i32 @llvm.fshr.i32(i32 %0, i32 %1, i32 0)
  ret i32 %2
}

; ============================================================
; i32 fshl/fshr - constant shamt=32 (32 & 31 = 0)
; ============================================================

define i32 @A3(i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 0
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, 32
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 true, i32 %0, i32 [[OR]]
; CHECK:  ret i32 [[SEL]]
  %2 = call i32 @llvm.fshl.i32(i32 %0, i32 %1, i32 32)
  ret i32 %2
}

define i32 @B3(i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 32
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, 0
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 true, i32 %1, i32 [[OR]]
; CHECK:  ret i32 [[SEL]]
  %2 = call i32 @llvm.fshr.i32(i32 %0, i32 %1, i32 32)
  ret i32 %2
}

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)