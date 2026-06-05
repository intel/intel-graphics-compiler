;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Test checks that GenXLowering lowers llvm.bswap of an illegal bit width by
; extending to the next legal type (16, 32 or 64 bits), performing the byte swap
; and then shifting and truncating back. Note: llvm.bswap requires an even
; number of bytes, so i48 is the only illegal width in the 16..64 bit range that
; is valid LLVM IR.

declare i48 @llvm.bswap.i48(i48)
declare <4 x i48> @llvm.bswap.v4i48(<4 x i48>)

define spir_func i48 @scalar_48(i48 %arg) {
; CHECK-LABEL: define spir_func i48 @scalar_48
; CHECK-NOT: call i48 @llvm.bswap.i48
; CHECK: %[[ZEXT48:[^ )]+]] = zext i48 %{{[^ )]+}} to i64
; CHECK: %[[LSHR48:[^ )]+]] = lshr i64 %{{[^ )]+}}, 16
; CHECK: %[[TRUNC48:[^ )]+]] = trunc i64 %[[LSHR48]] to i48
; CHECK: ret i48 %[[TRUNC48]]
  %res = tail call i48 @llvm.bswap.i48(i48 %arg)
  ret i48 %res
}

define spir_func <4 x i48> @vector_48(<4 x i48> %arg) {
; CHECK-LABEL: define spir_func <4 x i48> @vector_48
; CHECK-NOT: call <4 x i48> @llvm.bswap.v4i48
; CHECK: %[[VZEXT48:[^ )]+]] = zext <4 x i48> %{{[^ )]+}} to <4 x i64>
; CHECK: %[[VLSHR48:[^ )]+]] = lshr <4 x i64> %{{[^ )]+}}, <i64 16, i64 16, i64 16, i64 16>
; CHECK: %[[VTRUNC48:[^ )]+]] = trunc <4 x i64> %[[VLSHR48]] to <4 x i48>
; CHECK: ret <4 x i48> %[[VTRUNC48]]
  %res = tail call <4 x i48> @llvm.bswap.v4i48(<4 x i48> %arg)
  ret <4 x i48> %res
}

