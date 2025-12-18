;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXSimplify -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXSimplify -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; RUN: %opt_new_pm_typed -passes=GenXSimplify -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXSimplify -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i64> @llvm.genx.usmul.v2i64.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i64> @llvm.genx.sumul.v2i64.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <2 x i32> @llvm.genx.umulh.v2i32.v2i32(<2 x i32>, <2 x i32>)

; CHECK-LABEL: testi32ssv
; CHECK-NEXT: sext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: sext <2 x i32> %op2 to <2 x i64>
; CHECK-NEXT: call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; CHECK-NEXT: ret void
define internal spir_func void @testi32ssv(<2 x i32> %op1, <2 x i32> %op2) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %sop2 = sext <2 x i32> %op2 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, %sop2
  ret void
}
; CHECK-LABEL: testi32ssv_mm
; CHECK-NEXT: sext <2 x i8> %op1 to <2 x i64>
; CHECK-NEXT: sext <2 x i16> %op2 to <2 x i64>
; CHECK-NEXT: [[OP1:[^ ]+]] = sext <2 x i8> %op1 to <2 x i32>
; CHECK-NEXT: [[OP2:[^ ]+]] = sext <2 x i16> %op2 to <2 x i32>
; CHECK-NEXT: call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> [[OP1]], <2 x i32> [[OP2]])
; CHECK-NEXT: ret void
define internal spir_func void @testi32ssv_mm(<2 x i8> %op1, <2 x i16> %op2) {
  %sop1 = sext <2 x i8> %op1 to <2 x i64>
  %sop2 = sext <2 x i16> %op2 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, %sop2
  ret void
}

; CHECK-LABEL: testi32uuv
; CHECK-NEXT: zext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: zext <2 x i32> %op2 to <2 x i64>
; CHECK-NEXT: call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; CHECK-NEXT: ret void
define internal spir_func void @testi32uuv(<2 x i32> %op1, <2 x i32> %op2) {
  %sop1 = zext <2 x i32> %op1 to <2 x i64>
  %sop2 = zext <2 x i32> %op2 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, %sop2
  ret void
}
; COM: this case is disabled since we do not support signed/unsigned mixing yet
; COM: cHECK-LABEL: testi32usv
; COM: cHECK-NEXT: zext <2 x i32> %op1 to <2 x i64>
; COM: cHECK-NEXT: sext <2 x i32> %op2 to <2 x i64>
; COM: cHECK-NEXT: call <2 x i64> @llvm.genx.usmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; COM: cHECK-NEXT: ret void
define internal spir_func void @testi32usv(<2 x i32> %op1, <2 x i32> %op2) {
  %sop1 = zext <2 x i32> %op1 to <2 x i64>
  %sop2 = sext <2 x i32> %op2 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, %sop2
  ret void
}
; COM: this case is disabled since we do not support signed/unsigned mixing yet
; COM: cHECK-LABEL: testi32suv
; COM: cHECK-NEXT: sext <2 x i32> %op1 to <2 x i64>
; COM: cHECK-NEXT: zext <2 x i32> %op2 to <2 x i64>
; COM: cHECK-NEXT: call <2 x i64> @llvm.genx.sumul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; COM: cHECK-NEXT: ret void
define internal spir_func void @testi32suv(<2 x i32> %op1, <2 x i32> %op2) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %sop2 = zext <2 x i32> %op2 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, %sop2
  ret void
}

; CHECK-LABEL: testi32suvc
; CHECK-NEXT: sext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> <i32 1, i32 2>)
; CHECK-NEXT: ret void
define internal spir_func void @testi32suvc(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64 1, i64 2>
  ret void
}

; CHECK-LABEL: testi32suSc
; CHECK-NEXT: sext i32 %op1 to i64
; CHECK-NEXT: call i64 @llvm.genx.ssmul.i64.i32(i32 %op1, i32 3)
; CHECK-NEXT: ret void
define internal spir_func void @testi32suSc(i32 %op1) {
  %sop1 = sext i32 %op1 to i64
  %mul64 = mul i64 %sop1, 3
  ret void
}
; CHECK-LABEL: testi32suSc_fail
; CHECK-NEXT: %sop1 = sext i32 %op1 to i64
; CHECK-NEXT: %mul64 = mul i64 %sop1, 12147483648
; CHECK-NEXT: ret void
define internal spir_func void @testi32suSc_fail(i32 %op1) {
  %sop1 = sext i32 %op1 to i64
  %mul64 = mul i64 %sop1, 12147483648
  ret void
}

; COM: -2147483649 - should not be representable, INT32_MIN=-2147483648
; CHECK-LABEL: test_v_c_fail_1
; CHECK-NEXT: %sop1 = sext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: %mul64 = mul <2 x i64> %sop1, <i64 -2147483649, i64 2>
; CHECK-NEXT: ret void
define internal spir_func void @test_v_c_fail_1(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64 -2147483649, i64 2>
  ret void
}
; COM: 4294967296 - should not be representable, since UINT32_MAX=4294967295
; CHECK-LABEL: test_v_c_fail_2
; CHECK-NEXT: %sop1 = sext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: %mul64 = mul <2 x i64> %sop1, <i64  4294967295, i64 4294967296>
; CHECK-NEXT: ret void
define internal spir_func void @test_v_c_fail_2(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64  4294967295, i64 4294967296>
  ret void
}

; COM: INT32_MIN: -2147483648
; CHECK-LABEL: test_v_c_mached_1
; CHECK-NEXT: sext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> <i32 -2147483648, i32 2>)
; CHECK-NEXT: ret void
define internal spir_func void @test_v_c_mached_1(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64 -2147483648, i64 2>
  ret void
}
; COM: UINT32_MAX: 4294967295 -> 0xffffffff (-1 as int32_t)
; CHECK-LABEL: test_v_c_mached_2
; CHECK-NEXT: zext <2 x i32> %op1 to <2 x i64>
; CHECK-NEXT: call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> <i32 -1, i32 2>)
; CHECK-NEXT: ret void
define internal spir_func void @test_v_c_mached_2(<2 x i32> %op1) {
  %sop1 = zext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64  4294967295, i64 2>
  ret void
}
; COM: this case is disabled since we do not support signed/unsigned mixing yet
; COM: UINT32_MAX: 4294967295 -> 0xffffffff (-1 as int32_t)
; COM: cHECK-LABEL: test_v_c_mached_3
; COM: cHECK-NEXT: sext <2 x i32> %op1 to <2 x i64>
; COM: cHECK-NEXT: call <2 x i64> @llvm.genx.sumul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> <i32 -1, i32 2>)
; COM: cHECK-NEXT: ret void
define internal spir_func void @test_v_c_mached_3(<2 x i32> %op1) {
  %sop1 = sext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64 4294967295, i64 2>
  ret void
}
; COM: this case is disabled since we do not support signed/unsigned mixing yet
; COM: INT32_MIN: -2147483648
; COM: cHECK-LABEL: test_v_c_mached_4
; COM: cHECK-NEXT: zext <2 x i32> %op1 to <2 x i64>
; COM: cHECK-NEXT: call <2 x i64> @llvm.genx.usmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> <i32 -2147483648, i32 2>)
; COM: cHECK-NEXT: ret void
define internal spir_func void @test_v_c_mached_4(<2 x i32> %op1) {
  %sop1 = zext <2 x i32> %op1 to <2 x i64>
  %mul64 = mul <2 x i64> %sop1, <i64 -2147483648, i64 2>
  ret void
}
