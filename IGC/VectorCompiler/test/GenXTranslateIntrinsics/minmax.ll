;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% %pass_pref%GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.smax.v32i32.v32i32(<32 x i32>, <32 x i32>)
declare <32 x i32> @llvm.genx.smin.v32i32.v32i32(<32 x i32>, <32 x i32>)
declare <32 x i32> @llvm.genx.umax.v32i32.v32i32(<32 x i32>, <32 x i32>)
declare <32 x i32> @llvm.genx.umin.v32i32.v32i32(<32 x i32>, <32 x i32>)

declare <32 x i32> @llvm.genx.smax.v32i32.v32i16(<32 x i16>, <32 x i16>)
declare <32 x i32> @llvm.genx.smin.v32i32.v32i16(<32 x i16>, <32 x i16>)
declare <32 x i32> @llvm.genx.umax.v32i32.v32i16(<32 x i16>, <32 x i16>)
declare <32 x i32> @llvm.genx.umin.v32i32.v32i16(<32 x i16>, <32 x i16>)

declare <32 x i16> @llvm.genx.smax.v32i16.v32i32(<32 x i32>, <32 x i32>)
declare <32 x i16> @llvm.genx.smin.v32i16.v32i32(<32 x i32>, <32 x i32>)
declare <32 x i16> @llvm.genx.umax.v32i16.v32i32(<32 x i32>, <32 x i32>)
declare <32 x i16> @llvm.genx.umin.v32i16.v32i32(<32 x i32>, <32 x i32>)

; CHECK-LABEL: @test_same_width
define void @test_same_width(<32 x i32> %a, <32 x i32> %b) {
; CHECK: = call <32 x i32> @llvm.smax.v32i32(<32 x i32> %a, <32 x i32> %b)
  %1 = call <32 x i32> @llvm.genx.smax.v32i32.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: = call <32 x i32> @llvm.smin.v32i32(<32 x i32> %a, <32 x i32> %b)
  %2 = call <32 x i32> @llvm.genx.smin.v32i32.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: = call <32 x i32> @llvm.umax.v32i32(<32 x i32> %a, <32 x i32> %b)
  %3 = call <32 x i32> @llvm.genx.umax.v32i32.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: = call <32 x i32> @llvm.umin.v32i32(<32 x i32> %a, <32 x i32> %b)
  %4 = call <32 x i32> @llvm.genx.umin.v32i32.v32i32(<32 x i32> %a, <32 x i32> %b)
  ret void
}

; CHECK-LABEL: @test_ext
define void @test_ext(<32 x i16> %a, <32 x i16> %b) {
; CHECK: [[SMAX:[^ ]+]] = call <32 x i16> @llvm.smax.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[EXT1:[^ ]+]] = sext <32 x i16> [[SMAX]] to <32 x i32>
  %1 = call <32 x i32> @llvm.genx.smax.v32i32.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[SMIN:[^ ]+]] = call <32 x i16> @llvm.smin.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[EXT2:[^ ]+]] = sext <32 x i16> [[SMIN]] to <32 x i32>
  %2 = call <32 x i32> @llvm.genx.smin.v32i32.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[UMAX:[^ ]+]] = call <32 x i16> @llvm.umax.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[EXT3:[^ ]+]] = zext <32 x i16> [[UMAX]] to <32 x i32>
  %3 = call <32 x i32> @llvm.genx.umax.v32i32.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[UMIN:[^ ]+]] = call <32 x i16> @llvm.umin.v32i16(<32 x i16> %a, <32 x i16> %b)
; CHECK: [[EXT4:[^ ]+]] = zext <32 x i16> [[UMIN]] to <32 x i32>
  %4 = call <32 x i32> @llvm.genx.umin.v32i32.v32i16(<32 x i16> %a, <32 x i16> %b)
  ret void
}

; CHECK-LABEL: @test_trunc
define void @test_trunc(<32 x i32> %a, <32 x i32> %b) {
; CHECK: [[SMAX:[^ ]+]] = call <32 x i32> @llvm.smax.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[TRUNC1:[^ ]+]] = trunc <32 x i32> [[SMAX]] to <32 x i16>
  %1 = call <32 x i16> @llvm.genx.smax.v32i16.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[SMIN:[^ ]+]] = call <32 x i32> @llvm.smin.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[TRUNC2:[^ ]+]] = trunc <32 x i32> [[SMIN]] to <32 x i16>
  %2 = call <32 x i16> @llvm.genx.smin.v32i16.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[UMAX:[^ ]+]] = call <32 x i32> @llvm.umax.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[TRUNC3:[^ ]+]] = trunc <32 x i32> [[UMAX]] to <32 x i16>
  %3 = call <32 x i16> @llvm.genx.umax.v32i16.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[UMIN:[^ ]+]] = call <32 x i32> @llvm.umin.v32i32(<32 x i32> %a, <32 x i32> %b)
; CHECK: [[TRUNC4:[^ ]+]] = trunc <32 x i32> [[UMIN]] to <32 x i16>
  %4 = call <32 x i16> @llvm.genx.umin.v32i16.v32i32(<32 x i32> %a, <32 x i32> %b)
  ret void
}
