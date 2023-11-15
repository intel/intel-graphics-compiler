;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mattr=-mul_ddq -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mattr=+mul_ddq -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=UseMulDDQ %s

declare i32 @llvm.genx.ssmul.sat.i32.i32(i32, i32)
declare i32 @llvm.genx.uumul.sat.i32.i32(i32, i32)

; CHECK-LABEL: testi32s
; UseMulDDQ-LABEL: testi32s
define internal spir_func void @testi32s(i32 %op1, i32 %op2) {
; CHECK: call <16 x i32> @llvm.genx.smadw.v16i32.v1i32
; CHECK-NOT: mul <1 x i64>
; CHECK: %res.sat = call i32 @llvm.genx.sstrunc.sat.i32.i64(i64 %res1.recast)
; UseMulDDQ: call i64 @llvm.genx.ssmul.i64.i32
; UseMulDDQ: %res.sat = call i32 @llvm.genx.sstrunc.sat.i32.i64(i64 %res1)
  %res = call i32 @llvm.genx.ssmul.sat.i32.i32(i32 %op1, i32 %op2)
  ret void
}

; CHECK-LABEL: testi32u
; UseMulDDQ-LABEL: testi32u
define internal spir_func void @testi32u(i32 %op1, i32 %op2) {
; CHECK: call <16 x i32> @llvm.genx.umadw.v16i32.v1i32
; CHECK-NOT: mul <1 x i64>
; CHECK: %res.sat = call i32 @llvm.genx.uutrunc.sat.i32.i64(i64 %res1.recast)
; UseMulDDQ: call i64 @llvm.genx.uumul.i64.i32
; UseMulDDQ: %res.sat = call i32 @llvm.genx.uutrunc.sat.i32.i64(i64 %res1)
  %res = call i32 @llvm.genx.uumul.sat.i32.i32(i32 %op1, i32 %op2)
  ret void
}
