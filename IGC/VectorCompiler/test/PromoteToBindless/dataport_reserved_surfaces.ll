;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that old dataport intrinsics with fixed BTI are not converted.

; RUN: opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32)

declare void @sink.v8i32(<8 x i32>)
declare i32 @src.i32()

; CHECK-LABEL: @oword_ld(
; CHECK-NEXT: [[ADDR:%[^ ]*]] = call i32 @src.i32()
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 255, i32 [[ADDR]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @oword_ld() {
  %addr = call i32 @src.i32()
  %res = call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 255, i32 %addr)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}
