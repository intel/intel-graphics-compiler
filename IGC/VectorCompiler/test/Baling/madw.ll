;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.smadw.v32i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <32 x i32> @llvm.genx.umadw.v32i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)

; CHECK: GenXBaling dump start

; CHECK: bales in function: sext:
; CHECK-NOT: llvm.genx.smadw.v2i32{{[^)]*}}): maininst
define <32 x i32> @sext(<2 x i8> %val) {
  %sext = sext <2 x i8> %val to <2 x i32>
  %madw = call <32 x i32> @llvm.genx.smadw.v32i32.v2i32(<2 x i32> %sext, <2 x i32> <i32 3, i32 3>, <2 x i32> zeroinitializer)
  ret <32 x i32> %madw
}

; CHECK: bales in function: zext:
; CHECK-NOT: llvm.genx.umadw.v2i32{{[^)]*}}): maininst
define <32 x i32> @zext(<2 x i8> %val) {
  %zext = zext <2 x i8> %val to <2 x i32>
  %madw = call <32 x i32> @llvm.genx.umadw.v32i32.v2i32(<2 x i32> %zext, <2 x i32> <i32 3, i32 3>, <2 x i32> zeroinitializer)
  ret <32 x i32> %madw
}

; CHECK: GenXBaling dump end
