;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mattr=-mul_ddq -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mattr=+mul_ddq -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=UseMulDDQ %s
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=SIMD16 %s

; CHECK-LABEL: testi32suSc
; CHECK: call <16 x i32> @llvm.genx.umadw.v16i32.v1i32
; CHECK-COUNT-2: mul <1 x i32>
; SIMD16-LABEL: testi32suSc
; SIMD16: call <32 x i32> @llvm.genx.umadw.v32i32.v1i32
; SIMD16-COUNT-2: mul <1 x i32>
; UseMulDDQ-LABEL: testi32suSc
; UseMulDDQ: call i64 @llvm.genx.uumul.i64.v1i32
; UseMulDDQ-COUNT-2: mul <1 x i32>
define internal spir_func void @testi32suSc(i32 %op1) {
  %sop1 = sext i32 %op1 to i64
  %mul64 = mul i64 %sop1, 12147483648
  ret void
}

; CHECK-LABEL: test_v_c1
; CHECK: call <16 x i32> @llvm.genx.umadw.v16i32.v2i32
; CHECK-COUNT-2: mul <2 x i32>
; SIMD16-LABEL: test_v_c1
; SIMD16: call <32 x i32> @llvm.genx.umadw.v32i32.v2i32
; SIMD16-COUNT-2: mul <2 x i32>
; UseMulDDQ-LABEL: test_v_c1
; UseMulDDQ: call <2 x i64> @llvm.genx.uumul.v2i64.v2i32
; UseMulDDQ-COUNT-2: mul <2 x i32>
define internal spir_func void @test_v_c1(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64 -2147483649, i64 2>
  ret void
}

; COM: 4294967296 - should not be representable, since UINT32_MAX=4294967295
; CHECK-LABEL: test_v_c2
; CHECK: call <16 x i32> @llvm.genx.umadw.v16i32.v2i32
; CHECK-COUNT-2: mul <2 x i32>
; SIMD16-LABEL: test_v_c2
; SIMD16: call <32 x i32> @llvm.genx.umadw.v32i32.v2i32
; SIMD16-COUNT-2: mul <2 x i32>
; UseMulDDQ-LABEL: test_v_c2
; UseMulDDQ: call <2 x i64> @llvm.genx.uumul.v2i64.v2i32
; UseMulDDQ-COUNT-2: mul <2 x i32>
define internal spir_func void @test_v_c2(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64  4294967295, i64 4294967296>
  ret void
}
