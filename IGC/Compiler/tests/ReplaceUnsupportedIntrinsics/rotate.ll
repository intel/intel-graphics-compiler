;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test checks that fshl/fshr intrinsics are preserved on platforms with ror/rol 64bit support
; and replaced with equivalent operations on other platforms

; REQUIRES: llvm-14-plus
; RUN: igc_opt --platformpvc --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK64
; RUN: igc_opt --platformPtl --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK64
; RUN: igc_opt --platformmtl --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK32
; RUN: igc_opt --platformdg2 --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK32
; ------------------------------------------------
; ReplaceUnsupportedIntrinsics
; ------------------------------------------------

; CHECK64-LABEL: define i64 @test_rotate64(
; CHECK64: call i64 @llvm.fshl.i64(i64 %or, i64 %or, i64 63)
; CHECK64: call i64 @llvm.fshr.i64(i64 %or, i64 %or, i64 56)

; CHECK32-LABEL: define i64 @test_rotate64(
; CHECK32-NOT: call i64 @llvm.fshl.i64
; CHECK32-NOT: call i64 @llvm.fshr.i64
; CHECK32: [[SHL63:%[a-zA-Z0-9]+]] = shl i64 %or, 63
; CHECK32: [[LSHR1:%[a-zA-Z0-9]+]] = lshr i64 %or, 1
; CHECK32: [[ROTL63:%[a-zA-Z0-9]+]] = or i64 [[SHL63]], [[LSHR1]]
; CHECK32: [[SEL1:%[a-zA-Z0-9]+]] = select i1 false, i64 %or, i64 [[ROTL63]]
; CHECK32: [[SHL8:%[a-zA-Z0-9]+]] = shl i64 %or, 8
; CHECK32: [[LSHR56:%[a-zA-Z0-9]+]] = lshr i64 %or, 56
; CHECK32: [[ROTR56:%[a-zA-Z0-9]+]] = or i64 [[SHL8]], [[LSHR56]]
; CHECK32: [[SEL2:%[a-zA-Z0-9]+]] = select i1 false, i64 %or, i64 [[ROTR56]]
; CHECK32: add i64 [[SEL1]], [[SEL2]]

define i64 @test_rotate64(i32 %a, i32 %hi32, i32 %lo32, i32 %d) {
entry:
  %ext0 = zext i32 %a to i64
  %ext1 = zext i32 %hi32 to i64
  %shl = shl nuw i64 %ext1, 32
  %ext2 = zext i32 %lo32 to i64
  %or = or i64 %shl, %ext2
  %rot63 = call i64 @llvm.fshl.i64(i64 %or, i64 %or, i64 63)
  %rot56 = call i64 @llvm.fshr.i64(i64 %or, i64 %or, i64 56)
  %add1 = add i64 %rot63, %rot56
  %add2 = add i64 %add1, %ext0
  ret i64 %add2
}

; CHECK32-LABEL: define i32 @test_rotate32(
; CHECK32: call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 5)
; CHECK32: call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 5)

; CHECK64-LABEL: define i32 @test_rotate32(
; CHECK64: call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 5)
; CHECK64: call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 5)

define i32 @test_rotate32(i32 %x) {
entry:
  %rotl5 = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 5)
  %rotr5 = call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 5)
  %add = add i32 %rotl5, %rotr5
  ret i32 %add
}

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)
declare i64 @llvm.fshl.i64(i64, i64, i64)
declare i64 @llvm.fshr.i64(i64, i64, i64)
