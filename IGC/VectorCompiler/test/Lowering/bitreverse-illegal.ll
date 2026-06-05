;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Test checks that GenXLowering lowers llvm.bitreverse of an illegal bit width.
; Widths below 32 bits are extended to i32 and reversed via genx.bfrev. Widths
; between 33 and 63 bits are extended to i64 and reversed via the 64-bit path.

declare i9 @llvm.bitreverse.i9(i9)
declare i24 @llvm.bitreverse.i24(i24)
declare i33 @llvm.bitreverse.i33(i33)
declare i44 @llvm.bitreverse.i44(i44)
declare i48 @llvm.bitreverse.i48(i48)

; This narrow scalar reproduces the idiom seen in instcombine output of an
; ESIMD GEMM kernel: trunc i32 -> i9, llvm.bitreverse.i9, and-mask, zext back.
define spir_func i9 @scalar_9(i9 %arg) {
; CHECK-LABEL: define spir_func i9 @scalar_9
; CHECK-NOT: call i9 @llvm.bitreverse.i9
; CHECK: %[[ZEXT9:[^ )]+]] = zext i9 %{{[^ )]+}} to i32
; CHECK: call i32 @llvm.genx.bfrev.i32
; CHECK: %[[LSHR9:[^ )]+]] = lshr i32 %{{[^ )]+}}, 23
; CHECK: %[[TRUNC9:[^ )]+]] = trunc i32 %[[LSHR9]] to i9
; CHECK: ret i9 %[[TRUNC9]]
  %res = tail call i9 @llvm.bitreverse.i9(i9 %arg)
  ret i9 %res
}

define spir_func i24 @scalar_24(i24 %arg) {
; CHECK-LABEL: define spir_func i24 @scalar_24
; CHECK-NOT: call i24 @llvm.bitreverse.i24
; CHECK: %[[ZEXT24:[^ )]+]] = zext i24 %{{[^ )]+}} to i32
; CHECK: call i32 @llvm.genx.bfrev.i32
; CHECK: %[[LSHR24:[^ )]+]] = lshr i32 %{{[^ )]+}}, 8
; CHECK: %[[TRUNC24:[^ )]+]] = trunc i32 %[[LSHR24]] to i24
; CHECK: ret i24 %[[TRUNC24]]
  %res = tail call i24 @llvm.bitreverse.i24(i24 %arg)
  ret i24 %res
}

define spir_func i33 @scalar_33(i33 %arg) {
; CHECK-LABEL: define spir_func i33 @scalar_33
; CHECK-NOT: call i33 @llvm.bitreverse.i33
; CHECK: %[[ZEXT33:[^ )]+]] = zext i33 %{{[^ )]+}} to i64
; CHECK: %[[LSHR33:[^ )]+]] = lshr i64 %{{[^ )]+}}, 31
; CHECK: %[[TRUNC33:[^ )]+]] = trunc i64 %[[LSHR33]] to i33
; CHECK: ret i33 %[[TRUNC33]]
  %res = tail call i33 @llvm.bitreverse.i33(i33 %arg)
  ret i33 %res
}

define spir_func i44 @scalar_44(i44 %arg) {
; CHECK-LABEL: define spir_func i44 @scalar_44
; CHECK-NOT: call i44 @llvm.bitreverse.i44
; CHECK: %[[ZEXT44:[^ )]+]] = zext i44 %{{[^ )]+}} to i64
; CHECK: %[[LSHR44:[^ )]+]] = lshr i64 %{{[^ )]+}}, 20
; CHECK: %[[TRUNC44:[^ )]+]] = trunc i64 %[[LSHR44]] to i44
; CHECK: ret i44 %[[TRUNC44]]
  %res = tail call i44 @llvm.bitreverse.i44(i44 %arg)
  ret i44 %res
}

define spir_func i48 @scalar_48(i48 %arg) {
; CHECK-LABEL: define spir_func i48 @scalar_48
; CHECK-NOT: call i48 @llvm.bitreverse.i48
; CHECK: %[[ZEXT48:[^ )]+]] = zext i48 %{{[^ )]+}} to i64
; CHECK: %[[LSHR48:[^ )]+]] = lshr i64 %{{[^ )]+}}, 16
; CHECK: %[[TRUNC48:[^ )]+]] = trunc i64 %[[LSHR48]] to i48
; CHECK: ret i48 %[[TRUNC48]]
  %res = tail call i48 @llvm.bitreverse.i48(i48 %arg)
  ret i48 %res
}
