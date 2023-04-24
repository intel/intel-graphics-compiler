;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; COM: ===============================
; COM:      TEST #1:  icmp eq
; COM:      TEST #1.5:  icmp eq (scalar)
; COM:      TEST #2:  icmp ne
; COM:      TEST #3:  icmp ugt
; COM:      TEST #4:  icmp uge
; COM:      TEST #5:  icmp ult
; COM:      TEST #6:  icmp ule
; COM:      TEST #7:  icmp sgt
; COM:      TEST #8:  icmp sge
; COM:      TEST #9:  icmp slt
; COM:      TEST #10: icmp sle
; COM: ===============================

; COM: ===============================
; COM:      TEST #1 (icmp eq)
; COM: ===============================
; COM: `icmp eq` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. sel_lo = select eq src0.lo, src1.lo
; COM: 3. sel_hi = select eq src0.hi, src1.hi
; COM: 4. sel    = and sel_lo, sel_hi

; CHECK: @test_icmp_eq
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = and <8 x i1> [[Slo]], [[Shi]]
; COM: just check that the result is used
; CHECK: [[Res]]


define dllexport spir_kernel void @test_icmp_eq(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp eq <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #1.5 (icmp eq, scalar)
; COM: ===============================
; COM: `icmp eq` on scalar transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. sel_lo = select eq src0.lo, src1.lo
; COM: 3. sel_hi = select eq src0.hi, src1.hi
; COM: 4. sel    = and sel_lo, sel_hi
; COM: 4. sel.cast    = bitcast <1 x i1> sel, i1

; CHECK: @test_scalar_icmp_eq
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast i64 %scalar_left to <[[CT:2 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 1, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast i64 %scalar_right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = and <1 x i1> [[Slo]], [[Shi]]
; CHECK-NEXT: [[Casted:%[^ ]+]] = bitcast <1 x i1> [[Res]] to i1
; COM: just check that the result is used
; CHECK: [[Casted]]


define dllexport spir_kernel void @test_scalar_icmp_eq(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %scalar_left  = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %left, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %scalar_right = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %right, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %icmp = icmp eq i64 %scalar_left, %scalar_right
  %res = select i1 %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #2 (icmp ne)
; COM: ===============================
; COM: `icmp ne` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. sel_lo = select ne src0.lo, src1.lo
; COM: 3. sel_hi = select ne src0.hi, src1.hi
; COM: 4. sel    = or sel_hi, sel_lo

; CHECK: @test_icmp_ne
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp ne <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp ne <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[Slo]], [[Shi]]
; COM: just check that the result is used
; CHECK: [[Res]]

define dllexport spir_kernel void @test_icmp_ne(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp ne <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #3 (icmp ugt)
; COM: ===============================
; COM: `icmp ugt` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp ugt src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp ugt src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_ugt
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ugt <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp ugt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]
; COM: just check that the result is used
; CHECK: [[Res]]

define dllexport spir_kernel void @test_icmp_ugt(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp ugt <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #4 (icmp uge)
; COM: ===============================
; COM: `icmp uge` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp uge src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp ugt src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_uge
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[T0:%[^ ]+]] = icmp uge <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp ugt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]

define dllexport spir_kernel void @test_icmp_uge(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp uge <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #5 (icmp ult)
; COM: ===============================
; COM: `icmp ult` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp ult src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp ult src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_ult
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ult <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp ult <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]

define dllexport spir_kernel void @test_icmp_ult(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp ult <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #6 (icmp ule)
; COM: ===============================
; COM: `icmp ule` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp ule src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp ult src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_ule
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ule <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp ult <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]


define dllexport spir_kernel void @test_icmp_ule(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp ule <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #7 (icmp sgt)
; COM: ===============================
; COM: `icmp ugt` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp ugt src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp sgt src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_sgt
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ugt <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp sgt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]

define dllexport spir_kernel void @test_icmp_sgt(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp sgt <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #8 (icmp sge)
; COM: ===============================
; COM: `icmp sge` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp uge src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp sgt src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_sge
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp uge <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp sgt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]

define dllexport spir_kernel void @test_icmp_sge(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp sge <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #9 (icmp slt)
; COM: ===============================
; COM: `icmp slt` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp ult src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp slt src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_slt
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ult <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp slt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]

define dllexport spir_kernel void @test_icmp_slt(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp slt <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

; COM: ===============================
; COM:      TEST #10 (icmp sle)
; COM: ===============================
; COM: `icmp sle` transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. t0 = icmp ule src0.lo, src1.lo
; COM: 3. t1 = icmp eq  src0.hi, src1.hi
; COM: 4. t2 = and t1, t0
; COM: 5. t3 = icmp slt src0.hi, src1.hi
; COM: 6. res = or t2, t3

; CHECK: @test_icmp_sle
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ule <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <8 x i1> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp slt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <8 x i1> [[T2]], [[T3]]

define dllexport spir_kernel void @test_icmp_sle(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %icmp = icmp sle <8 x i64> %left, %right
  %res = select <8 x i1> %icmp, <8 x i64> %left, <8 x i64> %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %res)
  ret void
}

declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
declare { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
