;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=CHECK-LSC
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=CHECK-LSC-SIMD16

declare void @llvm.trap()

define spir_func void @test() {
; CHECK: [[PAYLOAD:%[^ ]+]] = call <8 x i32> @llvm.genx.r0.v8i32()
; CHECK: call void @llvm.genx.raw.send2.noresult.i1.v8i32(i8 2, i8 0, i1 true, i8 1, i8 7, i32 0, i32 33554448, <8 x i32> [[PAYLOAD]])
; CHECK-LSC: [[PAYLOAD:%[^ ]+]] = call <8 x i32> @llvm.genx.r0.v8i32()
; CHECK-LSC: call void @llvm.genx.raw.send2.noresult.i1.v8i32(i8 2, i8 0, i1 true, i8 1, i8 3, i32 0, i32 33554448, <8 x i32> [[PAYLOAD]])
; CHECK-LSC-SIMD16: [[PAYLOAD:%[^ ]+]] = call <8 x i32> @llvm.genx.r0.v8i32()
; CHECK-LSC-SIMD16: call void @llvm.genx.raw.send2.noresult.i1.v8i32(i8 2, i8 0, i1 true, i8 1, i8 3, i32 0, i32 33554448, <8 x i32> [[PAYLOAD]])
  tail call void @llvm.trap()
  ret void
}
