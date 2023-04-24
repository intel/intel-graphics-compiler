;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; COM: ===============================
; COM:         TEST #1
; COM: ===============================
; COM: 1. operands are splitted to left/right parts
; COM: 2. not_left = not (left) (xor with -1)
; COM: 3. not_right = not (right) (xor with -1)
; COM: 4. not  = combine(op_left,op_right)

; CHECK: @kernel_not
; CHECK: [[IV1:%[^ ]*.cast[0-9]*]] = bitcast <[[ET:8 x i64]]> %val to <[[CT:16 x i32]]>
; CHECK: [[IV2:%[^ ]*.not[0-9]*]] = xor <[[CT]]> [[IV1]], <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>
; CHECK: [[IV3:%[^ ]*.cast[0-9]*]] = bitcast <[[CT]]> [[IV2]] to <[[ET]]>

define dllexport spir_kernel void @kernel_not(i32 %0, i32 %1, i32 %2) {
  %val = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %not = xor <8 x i64> %val, <i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1>
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %not)
  ret void
}
; COM: ===============================
; COM:         TEST #2
; COM: ===============================
; CHECK: @kernel_not_reversed
; CHECK: [[IV1:%[^ ]*.cast[0-9]*]] = bitcast <[[ET:8 x i64]]> %val to <[[CT:16 x i32]]>
; CHECK: [[IV2:%[^ ]*.not[0-9]*]] = xor <[[CT]]> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, [[IV1]]
; CHECK: [[IV3:%[^ ]*.cast[0-9]*]] = bitcast <[[CT]]> [[IV2]] to <[[ET]]>
define dllexport spir_kernel void @kernel_not_reversed(i32 %0, i32 %1, i32 %2) {
  %val = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %not = xor <8 x i64> <i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1>, %val
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %not)
  ret void
}

declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
declare { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
