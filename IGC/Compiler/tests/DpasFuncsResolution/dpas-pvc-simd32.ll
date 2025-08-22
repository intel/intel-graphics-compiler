;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: system-windows
; REQUIRES: debug, llvm-14-plus
; RUN: not igc_opt --opaque-pointers -platformpvc --igc-arith-funcs-translation -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DpasFuncsResolution
; ------------------------------------------------

; Check assertion unique to DPAS in double-subgroup size.

; CHECK: RC >= 2, ICE: repeat count of DPAS for double subgroup-size must be >= 2!

define spir_kernel void @test_dpas(<4 x i32> %src, i32 %src2, ptr %dst) {
  %1 = load i16, ptr %dst, align 4
  %2 = call i32 @__builtin_IB_sub_group32n16_idpas_s8_s8_8_1(i32 %src2, i16 %1, <4 x i32> %src)
  store i32 %2, ptr %dst, align 4
  ret void
}

declare i32 @__builtin_IB_sub_group32n16_idpas_s8_s8_8_1(i32, i16, <4 x i32>)
