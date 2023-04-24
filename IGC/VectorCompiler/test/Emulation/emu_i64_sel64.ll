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
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. lo = select(cond, TrueVal.lo, FalseVal.lo)
; COM: 2. hi = select(cond, TrueVal.hi, FalseVal.hi)
; COM: 4. seli64  = combine(lo, hi)

; CHECK: @kernel_select_scalar_cond
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_T:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_T:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_F:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_F:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: %sel.lo = select i1 true, <[[ET]]> [[Lo_T]], <[[ET]]> [[Lo_F]]
; CHECK-NEXT: %sel.hi = select i1 true, <[[ET]]> [[Hi_T]], <[[ET]]> [[Hi_F]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> %sel.lo, [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> %sel.hi, [[high_reg]]
; CHECK-NEXT: {{[^ ]+}} = bitcast <[[CT]]> [[JOINED]] to <8 x i64>

define dllexport spir_kernel void @kernel_select_scalar_cond(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %sel = select i1 true, <8 x i64> %left,  <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %sel)
  ret void
}

; COM: ===============================
; COM:         TEST #2
; COM: ===============================

; CHECK: @kernel_select_vector_cond
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[I64T:8 x i64]]> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_T:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_T:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_F:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_F:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: %sel.lo = select <[[COND_TYPE:8 x i1]]> <[[COND:[^>]+]]>, <[[ET]]> [[Lo_T]], <[[ET]]> [[Lo_F]]
; CHECK-NEXT: %sel.hi = select <[[COND_TYPE]]> <[[COND]]>, <[[ET]]> [[Hi_T]], <[[ET]]> [[Hi_F]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> %sel.lo, [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> %sel.hi, [[high_reg]]
; CHECK-NEXT: {{[^ ]+}} = bitcast <[[CT]]> [[JOINED]] to <[[I64T]]>

define dllexport spir_kernel void @kernel_select_vector_cond(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %sel = select <8 x i1> <i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true>, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %sel)
  ret void
}

; COM: ===============================
; COM:         TEST #3
; COM: ===============================
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. lo = select(cond, TrueVal.lo, FalseVal.lo)
; COM: 2. hi = select(cond, TrueVal.hi, FalseVal.hi)
; COM: 4. seli64  = combine(lo, hi)

; CHECK: @kernel_select_scalar_cond_const

; CHECK-NEXT: %sel.lo = select i1 %pred, <[[ET:1 x i32]]> <i32 1>, <[[ET]]> zeroinitializer
; CHECK-NEXT: %sel.hi = select i1 %pred, <[[ET]]> zeroinitializer, <[[ET]]> zeroinitializer

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:2 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> %sel.lo, i32 0, i32 1, i32 2, i16 0,
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> %sel.hi, i32 0, i32 1, i32 2, i16 4,
; CHECK-NEXT: [[VVAL:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <1 x i64>
; CHECK-NEXT: [[SVAL:%[^ ]+]] = bitcast <1 x i64> [[VVAL]] to i64
; CHECK-NEXT: tail call void @llvm.genx.oword.st.i64(i32 %2, i32 0, i64 [[SVAL]])

define dllexport spir_kernel void @kernel_select_scalar_cond_const(i32 %0, i32 %1, i32 %2, i1 %pred) {
  %sel = select i1 %pred, i64 1,  i64 0
  tail call void @llvm.genx.oword.st.i64(i32 %2, i32 0, i64 %sel)
  ret void
}

; COM: ===============================
; COM:         TEST #4
; COM: ===============================

; CHECK: @kernel_select_vector_cond_const

; CHECK-NEXT: %sel.lo = select <[[COND_TYPE:8 x i1]]> %pred, <[[ET:8 x i32]]> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <[[ET]]> zeroinitializer
; CHECK-NEXT: %sel.hi = select <[[COND_TYPE]]> %pred, <[[ET]]> zeroinitializer, <[[ET]]> zeroinitializer

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:16 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> %sel.lo, [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> %sel.hi, [[high_reg]]
; CHECK-NEXT: {{[^ ]+}} = bitcast <[[CT]]> [[JOINED]] to <[[I64T]]>

define dllexport spir_kernel void @kernel_select_vector_cond_const(i32 %0, i32 %1, i32 %2, <8 x i1> %pred) {
  %sel = select <8 x i1> %pred, <8 x i64> <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>, <8 x i64> zeroinitializer
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %sel)
  ret void
}

declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
declare { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
declare void @llvm.genx.oword.st.i64(i32, i32, i64)
