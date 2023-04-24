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
; COM:             TEST #1
; COM: ===============================
; COM: sub64 transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. [sub_lo, borrow] = genx_subb(src0.l0, src1.lo)
; COM: 3. sub_hi = genx_add3(src0.hi, -borrow, -src1.hi)
; COM: 4. sub64  = combine(sub_lo, sub_hi)

; CHECK: @test_kernel
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[SUBB:%[^ ]+]] = call { <[[ET]]>, <[[ET]]> } @llvm.genx.subb.{{[^(]+}}(<[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]])
; CHECK-NEXT: [[SUBB_SUB:%[^ ]+]] = extractvalue { <[[ET]]>, <[[ET]]> } [[SUBB]], 1
; CHECK-NEXT: [[SUBB_BORROW:%[^ ]+]] = extractvalue { <[[ET]]>, <[[ET]]> } [[SUBB]], 0
; CHECK-NEXT: [[BORROW_NEGATE:%[^ ]+]] = sub <[[ET]]> zeroinitializer, [[SUBB_BORROW]]
; CHECK-NEXT: [[Hi_r_negate:%negative.[^ ]+]] = sub <[[ET]]> zeroinitializer, [[Hi_r]]
; CHECK-NEXT: [[Sub_Hi_Part:%sub_hi.part]] = add <[[ET]]> [[Hi_l]], [[BORROW_NEGATE]]
; CHECK-NEXT: [[Sub_Hi:%[^ ]+]] = add <[[ET]]> [[Sub_Hi_Part]], [[Hi_r_negate]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[SUBB_SUB]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Sub_Hi]], [[high_reg]]

define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %sub64 = sub <8 x i64> %left, %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %sub64)
  ret void
}


; COM: ===============================
; COM:            TEST #2
; COM: ===============================
; COM: check that if we have a scalar operation - it shall be recasted to vector
; COM: and then back to scalar

; CHECK: @scalar_kernel
; CHECK: %scalar_left = tail call i64 @llvm.genx.rdregioni.i64.
; CHECK-NEXT: %scalar_right = tail call i64 @llvm.genx.rdregioni.i64.
; CHECK-NEXT: %sub64.iv32cast = bitcast i64 %scalar_left to <2 x i32>
; CHECK: bitcast i64 %scalar_right to <2 x i32>
; CHECK: [[SUB:%[^ ]+]] = bitcast <2 x i32> [[JOINED]] to <1 x i64>
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <1 x i64> [[SUB]] to i64


define dllexport spir_kernel void @scalar_kernel(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %scalar_left  = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %left, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %scalar_right = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %right, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %sub64 = sub i64 %scalar_left, %scalar_right
  ret void
}

declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
declare { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
