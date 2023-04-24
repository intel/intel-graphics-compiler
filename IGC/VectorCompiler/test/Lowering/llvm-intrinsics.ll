;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare i16 @llvm.bitreverse.i16(i16)
declare i32 @llvm.bitreverse.i32(i32)
declare <8 x i16> @llvm.bitreverse.v8i16(<8 x i16>)
declare <8 x i32> @llvm.bitreverse.v8i32(<8 x i32>)

; COM: note: originally was trunc, but it was lowered too
define i16 @test_bitreverse_i16(i16 %input)
{
; CHECK: [[ZEXT:%[^ ]+]] = zext i16 %input to i32
; CHECK-NEXT: [[BFREF:%[^ ]+]] = call i32 @llvm.genx.bfrev.i32(i32 [[ZEXT]])
; CHECK-NEXT: [[LSH:%[^ ]+]] = lshr i32 [[BFREF]], 16
; CHECK-NEXT: [[BCAST:%[^ ]+]] = bitcast i32 [[LSH]] to <2 x i16>
; CHECK-NEXT: [[RES:%[^ ]+]] = call i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16> [[BCAST]], i32 2, i32 1, i32 2, i16 0, i32 undef)

  %ret = call i16 @llvm.bitreverse.i16(i16 %input);
  ret i16 %ret
}
define i32 @test_bitreverse_i32(i32 %input)
{
; CHECK: [[BFREF:%[^ ]+]] = call i32 @llvm.genx.bfrev.i32(i32 %input)
  %ret = call i32 @llvm.bitreverse.i32(i32 %input);
  ret i32 %ret
}
define <8 x i16> @test_bitreverse_v8i16(<8 x i16> %input)
{
; CHECK: [[ZEXT:%[^ ]+]] = zext <8 x i16> %input to <8 x i32>
; CHECK-NEXT: [[BFREF:%[^ ]+]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> [[ZEXT]])
; CHECK-NEXT: [[LSH:%[^ ]+]] = lshr <8 x i32> [[BFREF]], <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
; CHECK-NEXT: [[BCAST:%[^ ]+]] = bitcast <8 x i32> [[LSH]] to <16 x i16>
; CHECK-NEXT: [[RES:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> [[BCAST]], i32 16, i32 8, i32 2, i16 0, i32 undef)
  %ret = call <8 x i16> @llvm.bitreverse.v8i16(<8 x i16> %input);
  ret <8 x i16> %ret
}
define <8 x i32> @test_bitreverse_v8i32(<8 x i32> %input)
{
; CHECK: [[BFREF:%[^ ]+]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> %input)
  %ret = call <8 x i32> @llvm.bitreverse.v8i32(<8 x i32> %input);
  ret <8 x i32> %ret
}

declare i32 @llvm.usub.sat.i32(i32 %0, i32 %1)
declare i32 @llvm.uadd.sat.i32(i32 %0, i32 %1)

define i32 @test_usub_sat(i32 %a, i32 %b)
{
  %res = tail call i32 @llvm.usub.sat.i32(i32 %a, i32 %b)
; COM: this one is in terms on uuadd.sat
; CHECK: [[NOTVAL:%[^ ]+]] = sub i32 0, %b
; CHECK: call i32 @llvm.genx.usadd.sat.i32.i32(i32 %a, i32 [[NOTVAL]])
  ret i32 %res
}

define i32 @test_uadd_sat(i32 %a, i32 %b)
{
  %res = tail call i32 @llvm.uadd.sat.i32(i32 %a, i32 %b)
; COM this one is 1-to-1
; CHECK: call i32 @llvm.genx.uuadd.sat.i32.i32(i32 %a, i32 %b)
  ret i32 %res
}
