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

; COM: NOTE: Since general cases for shifts are relatively complex - there is
; COM: little to befit to cover the exact expansion. The functionality should
; COM: tested by a proper EtoE tests. However, an "optimized" routes must
; COM: be covered

; COM: ===============================
; COM:      TEST #1:  shl (opt32)
; COM:  left shift on 32 are transformed as:
; COM:  1. hi = Op.lo
; COM:  2. lo = 0
; COM:  3. result = combine(lo, hi)
; COM: ===============================
; CHECK: @test_shl_opt32

; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> zeroinitializer, [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[Lo]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>

define dllexport spir_kernel void @test_shl_opt32(<2 x i64> %vop) {
  %v2 = shl <2 x i64> %vop, <i64 32, i64 32>
  ret void
}
; COM: ===============================
; COM:      TEST #2:  shl (opt32)
; COM:  left shift greater 32 are transformed as:
; COM:  1. hi = Op << (Sha - 32)
; COM:  2. lo = 0
; COM:  3. result = combine(lo, hi)
; COM: ===============================
; CHECK: @test_shl_opt_large
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Sh:%[^.]+]] = shl <[[ET]]> [[Lo]], <i32 31, i32 1>
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> zeroinitializer, [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[Sh]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_shl_opt_large(<2 x i64> %vop) {
  %v2 = shl <2 x i64> %vop, <i64 63, i64 33>
  ret void
}

; COM: ===============================
; COM:      TEST #3:  shl (opt32)
; COM:  left shift less than 32 is transformed as:
; COM:  1. lo = Op.lo << Sha
; COM:  2. hi1 = Op.hi << Sha
; COM:  3. hi2 = Op.hi *ASHR* [ConstExpr:(32 - Sha)]
; COM:  4. hi = or hi1, hi2
; COM:  3. result = combine(lo, hi)
; COM: ===============================
; CHECK: @test_shl_opt_small
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[ShLo:%[^ ]+]] = shl <[[ET]]> [[Lo]], <i32 1, i32 2>
; CHECK-NEXT: [[ShHi1:%[^ ]+]] = shl <[[ET]]> [[Hi]], <i32 1, i32 2>
; CHECK-NEXT: [[ShHi2:%[^ ]+]] = lshr <[[ET]]> [[Lo]], <i32 31, i32 30>
; CHECK-NEXT: [[ShHi:%[^ ]+]] = or <[[ET]]> [[ShHi1]], [[ShHi2]]
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<[[CT]]> undef, <[[ET]]> [[ShLo]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[ShHi]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_shl_opt_small(<2 x i64> %vop) {
  %v2 = shl <2 x i64> %vop, <i64 1, i64 2>
  ret void
}

; COM: ===============================
; COM:      TEST #4:  ashr (opt32)
; COM:  Arithmetic Shift Right on 32 are transformed as:
; COM: ===============================
; CHECK: @test_ashr_opt32
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[SrcHi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Hi2:%[^ ]+]] = ashr <[[ET]]> [[SrcHi]], <i32 31, i32 31>
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> [[SrcHi]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[Hi2]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>

define dllexport spir_kernel void @test_ashr_opt32(<2 x i64> %vop) {
  %v2 = ashr <2 x i64> %vop, <i64 32, i64 32>
  ret void
}
; COM: ===============================
; COM:      TEST #5:  ashr (opt32)
; COM:  Arithmetic Shift Right greater 32 are transformed as:
; COM: ===============================
; CHECK: @test_ashr_opt_large
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[SrcLo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[SrcHi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Lo:%[^ ]+]] = ashr <[[ET]]> [[SrcHi]], <i32 31, i32 1>
; CHECK-NEXT: [[Hi:%[^ ]+]] = ashr <[[ET]]> [[SrcHi]], <i32 31, i32 31>
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_ashr_opt_large(<2 x i64> %vop) {
  %v2 = ashr <2 x i64> %vop, <i64 63, i64 33>
  ret void
}

; COM: ===============================
; COM:      TEST #6:  ashr (opt32)
; COM:  Arithmetic Shift Right less 32 is transformed as:
; COM: ===============================
; CHECK: @test_ashr_opt_small
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[SrcLo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[SrcHi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Lo1:%[^ ]+]] = lshr <[[ET]]> [[SrcLo]], <i32 1, i32 2>
; CHECK-NEXT: [[Hi:%[^ ]+]] = ashr <[[ET]]> [[SrcHi]], <i32 1, i32 2>
; CHECK-NEXT: [[Lo2:%[^ ]+]] = shl <[[ET]]> [[SrcHi]], <i32 31, i32 30>
; CHECK-NEXT: [[Lo:%[^ ]+]] = or <[[ET]]> [[Lo1]], [[Lo2]]
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_ashr_opt_small(<2 x i64> %vop) {
  %v2 = ashr <2 x i64> %vop, <i64 1, i64 2>
  ret void
}

; COM: ===============================
; COM:      TEST #7:  lshr (opt32)
; COM:  Logical Shift Right on 32 are transformed as:
; COM: ===============================
; CHECK: @test_lshr_opt32
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> [[Hi]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> zeroinitializer, [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_lshr_opt32(<2 x i64> %vop) {
  %v2 = lshr <2 x i64> %vop, <i64 32, i64 32>
  ret void
}
; COM: ===============================
; COM:      TEST #8:  lshr (opt32)
; COM:  Shift Logical Right greater 32 are transformed as:
; COM: ===============================
; CHECK: @test_lshr_opt_large
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[SrcLo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[SrcHi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Lo:%[^ ]+]] = lshr <[[ET]]> [[SrcHi]], <i32 31, i32 1>
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> zeroinitializer, [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_lshr_opt_large(<2 x i64> %vop) {
  %v2 = lshr <2 x i64> %vop, <i64 63, i64 33>
  ret void
}

; COM: ===============================
; COM:      TEST #9:  lshr (opt32)
; COM:  Logical Shift Right less than 32 is transformed as:
; COM: ===============================
; CHECK: @test_lshr_opt_small
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <[[OT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[SrcLo:%[^.]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[SrcHi:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[Lo1:%[^ ]+]] = lshr <[[ET]]> [[SrcLo]], <i32 1, i32 2>
; CHECK-NEXT: [[Hi:%[^ ]+]] = lshr <[[ET]]> [[SrcHi]], <i32 1, i32 2>
; CHECK-NEXT: [[Lo2:%[^ ]+]] = shl <[[ET]]> [[SrcHi]], <i32 31, i32 30>
; CHECK-NEXT: [[Lo:%[^ ]+]] = or <[[ET]]> [[Lo1]], [[Lo2]]
; CHECK-NEXT: [[Partial:%[^ ]+]] = call <[[CT]]> [[wrgn:@llvm.genx.wrregioni.[^(]+]](<4 x i32> undef, <2 x i32> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[Joined:%[^ ]+]] = call <[[CT]]> [[wrgn]](<[[CT]]> [[Partial]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[Result:%[^ ]+]] = bitcast <[[CT]]> [[Joined]] to <[[OT]]>
define dllexport spir_kernel void @test_lshr_opt_small(<2 x i64> %vop) {
  %v2 = lshr <2 x i64> %vop, <i64 1, i64 2>
  ret void
}
