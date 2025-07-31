;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG \
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


declare i64 @llvm.bitreverse.i64(i64)
declare <8 x i64> @llvm.bitreverse.v8i64(<8 x i64>)

; CHECK-LABEL: test_bitreverse_i64
define i64 @test_bitreverse_i64(i64 %input)
{
; CHECK: [[CAST:%[^ ]+]] = bitcast i64 %input to <2 x i32>
; CHECK: [[LO_SPLIT:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[CAST]], i32 0, i32 1, i32 2, i16 0
; CHECK: [[HI_SPLIT:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[CAST]], i32 0, i32 1, i32 2, i16 4
; CHECK: [[REV_LO:%[^ ]+]] = call <1 x i32> @llvm.genx.bfrev.v1i32(<1 x i32> [[LO_SPLIT]])
; CHECK: [[REV_HI:%[^ ]+]] = call <1 x i32> @llvm.genx.bfrev.v1i32(<1 x i32> [[HI_SPLIT]])
; CHECK: [[JOIN_LO:%[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> [[REV_LO]], i32 0, i32 1, i32 2, i16 0
; CHECK: [[JOIN_HI_LO:%[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> [[JOIN_LO]], <1 x i32> [[REV_HI]], i32 0, i32 1, i32 2, i16 4
; CHECK: [[RET:%[^ ]+]] = bitcast <2 x i32> [[JOIN_HI_LO]] to <1 x i64>
; CHECK: [[RET_FINAL:%[^ ]+]] = bitcast <1 x i64> [[RET]] to i64
; CHECK: ret i64 [[RET_FINAL]]
  %ret = call i64 @llvm.bitreverse.i64(i64 %input);
  ret i64 %ret
}

; CHECK-LABEL: test_bitreverse_v8i64
define <8 x i64> @test_bitreverse_v8i64(<8 x i64> %input)
{
; CHECK: [[CAST:%[^ ]+]] = bitcast <8 x i64> %input to <16 x i32>
; CHECK: [[LO_SPLIT:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CAST]], i32 0, i32 8, i32 2, i16 0
; CHECK: [[HI_SPLIT:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CAST]], i32 0, i32 8, i32 2, i16 4
; CHECK: [[LO_REV:%[^ ]+]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> [[LO_SPLIT]])
; CHECK: [[HI_REV:%[^ ]+]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> [[HI_SPLIT]])
; CHECK: [[LO_JOIN:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[LO_REV]], i32 0, i32 8, i32 2, i16 0
; CHECK: [[JOIN:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[LO_JOIN]], <8 x i32> [[HI_REV]], i32 0, i32 8, i32 2, i16 4
; CHECK: [[RET:%[^ ]+]] = bitcast <16 x i32> [[JOIN]] to <8 x i64>
; CHECK: ret <8 x i64> [[RET]]
  %ret = call <8 x i64> @llvm.bitreverse.v8i64(<8 x i64> %input);
  ret <8 x i64> %ret
}

declare i32 @llvm.usub.sat.i32(i32 %0, i32 %1)
declare i32 @llvm.uadd.sat.i32(i32 %0, i32 %1)

define i32 @test_usub_sat(i32 %a, i32 %b)
{
  %res = tail call i32 @llvm.usub.sat.i32(i32 %a, i32 %b)
; COM: this one is in terms on uuadd.sat
; CHECK: [[NOTVAL:%[^ ]+]] = sub i32 0, %b
; CHECK: call i32 @llvm.vc.internal.add.uus.sat.i32.i32(i32 %a, i32 [[NOTVAL]])
  ret i32 %res
}

define i32 @test_uadd_sat(i32 %a, i32 %b)
{
  %res = tail call i32 @llvm.uadd.sat.i32(i32 %a, i32 %b)
; COM this one is 1-to-1
; CHECK: call i32 @llvm.genx.uuadd.sat.i32.i32(i32 %a, i32 %b)
  ret i32 %res
}

declare <8 x i32> @llvm.cttz.v8i32(<8 x i32>, i1)
declare <8 x i32> @llvm.ctlz.v8i32(<8 x i32>, i1)

; CHECK-LABEL: cttz_ctlz_int
define internal spir_func void @cttz_ctlz_int(<8 x i32> %arg) {
; CHECK: [[REV:%.*]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> %arg)
; CHECK:  @llvm.ctlz.v8i32(<8 x i32> [[REV]], i1 false)
  %1 = call <8 x i32>  @llvm.cttz.v8i32(<8 x i32> %arg, i1 false)
; CHECK: @llvm.ctlz.v8i32(<8 x i32> %arg, i1 false)
  %2 = call <8 x i32>  @llvm.ctlz.v8i32(<8 x i32> %arg, i1 false)
  ret void
}

declare <8 x i64> @llvm.cttz.v8i64(<8 x i64>, i1)

; CHECK-LABEL: cttz_vec64
define internal spir_func <8 x i64> @cttz_vec64(<8 x i64> %arg) {
; CHECK: [[CAST32:%.*]] = bitcast <8 x i64> %arg to <16 x i32>
; CHECK: [[LO_SPLIT:%.*]] =  call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CAST32]], i32 0, i32 8, i32 2, i16 0,
; CHECK: [[HI_SPLIT:%.*]] =  call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CAST32]], i32 0, i32 8, i32 2, i16 4,
; CHECK: [[REV_LO:%.*]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> [[LO_SPLIT]])
; CHECK: [[REV_HI:%.*]] = call <8 x i32> @llvm.genx.bfrev.v8i32(<8 x i32> [[HI_SPLIT]])
; CHECK: [[JOIN1:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[REV_LO]], i32 0, i32 8, i32 2, i16 0
; CHECK: [[JOIN2:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[JOIN1]], <8 x i32> [[REV_HI]], i32 0, i32 8, i32 2, i16 4
; CHECK: [[CAST:%.*]] = bitcast <16 x i32> [[JOIN2]] to <8 x i64>
; CHECK: [[RET:%.*]] = call <8 x i64> @llvm.ctlz.v8i64(<8 x i64> [[CAST]], i1 false)
; CHECK: ret <8 x i64> [[RET]]
  %ret = call <8 x i64>  @llvm.cttz.v8i64(<8 x i64> %arg, i1 false)
  ret <8 x i64> %ret
}

declare i64 @llvm.cttz.i64(i64, i1)

; CHECK-LABEL: cttz_64
define internal spir_func i64 @cttz_64(i64 %arg) {
; CHECK: [[CAST32:%.*]] = bitcast i64 %arg to <2 x i32>
; CHECK: [[LO_SPLIT:%.*]] =  call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[CAST32]], i32 0, i32 1, i32 2, i16 0
; CHECK: [[HI_SPLIT:%.*]] =  call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[CAST32]], i32 0, i32 1, i32 2, i16 4
; CHECK: [[REV_LO:%.*]] = call <1 x i32> @llvm.genx.bfrev.v1i32(<1 x i32> [[LO_SPLIT]])
; CHECK: [[REV_HI:%.*]] = call <1 x i32> @llvm.genx.bfrev.v1i32(<1 x i32> [[HI_SPLIT]])
; CHECK: [[JOIN1:%.*]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> [[REV_LO]], i32 0, i32 1, i32 2, i16 0
; CHECK: [[JOIN2:%.*]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> [[JOIN1]], <1 x i32> [[REV_HI]], i32 0, i32 1, i32 2, i16 4
; CHECK: [[CAST_V:%.*]] = bitcast <2 x i32> [[JOIN2]] to <1 x i64>
; CHECK: [[CAST:%.*]] = bitcast <1 x i64> [[CAST_V]] to i64
; CHECK: [[RET:%.*]] = call i64 @llvm.ctlz.i64(i64 [[CAST]], i1 false)
; CHECK: ret i64 [[RET]]
  %ret = call i64 @llvm.cttz.i64(i64 %arg, i1 false)
  ret i64 %ret
}

declare <3 x i8> @llvm.cttz.v3i8(<3 x i8>, i1)
declare <3 x i8> @llvm.ctlz.v3i8(<3 x i8>, i1)

; CHECK-LABEL: cttz_ctlz_vi8
define internal spir_func void @cttz_ctlz_vi8(<3 x i8> %arg) {
; CHECK: [[ZEXT:%.*]] = zext <3 x i8> %arg to <3 x i32>
; CHECK: [[REV:%.*]] = call <3 x i32> @llvm.genx.bfrev.v3i32(<3 x i32> [[ZEXT]])
; ...
; CHECK: [[CTTZ:%.*]] = call <3 x i32> @llvm.ctlz.v3i32({{.*}}, i1 false)
; CHECK: [[SUB:%.*]] = sub <3 x i32> [[CTTZ]], <i32 24, i32 24, i32 24>
; CHECK: bitcast <3 x i32> [[SUB]] to <12 x i8>
; ...
  %1 = call <3 x i8>  @llvm.cttz.v3i8(<3 x i8> %arg, i1 false)
; CHECK: [[ZEXT:%.*]] = zext <3 x i8> %arg to <3 x i32>
; CHECK: [[CTLZ:%.*]] = call <3 x i32> @llvm.ctlz.v3i32(<3 x i32> [[ZEXT]], i1 false)
; CHECK: [[SUB:%.*]] = sub <3 x i32> [[CTLZ]], <i32 24, i32 24, i32 24>
; CHECK: [[CAST:%.*]] = bitcast <3 x i32> [[SUB]] to <12 x i8>
; CHECK: call <3 x i8> @llvm.genx.rdregioni.v3i8.v12i8.i16(<12 x i8> [[CAST]], i32 12, i32 3, i32 4, i16 0, i32 undef)
  %2 = call <3 x i8>  @llvm.ctlz.v3i8(<3 x i8> %arg, i1 false)
  ret void
}

declare i16 @llvm.cttz.i16(i16, i1)
declare i16 @llvm.ctlz.i16(i16, i1)

; CHECK-LABEL: cttz_ctlz_i16
define internal spir_func void @cttz_ctlz_i16(i16 %arg) {
; CHECK: [[ZEXT_1:%.*]] = zext i16 %arg to i32
; CHECK: [[REV:%.*]] = call i32 @llvm.genx.bfrev.i32(i32 [[ZEXT_1]])
; CHECK: [[SHIFT:%.*]] = lshr i32 [[REV]], 16
; CHECK: [[DOWNCAST:%.*]] = bitcast i32 [[SHIFT]] to <2 x i16>
; CHECK: [[RDREG_1:%.*]] = call i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16> [[DOWNCAST]], i32 2, i32 1, i32 2, i16 0, i32 undef)
; CHECK: [[ZEXT_2:%.*]] = zext i16 [[RDREG_1]] to i32
; CHECK: [[CTLZ:%.*]] = call i32 @llvm.ctlz.i32(i32 [[ZEXT_2]], i1 false)
; CHECK: [[SUB:%.*]] = sub i32 [[CTLZ]], 16
; CHECK: [[DOWNCAST:%.*]] = bitcast i32 [[SUB]] to <2 x i16>
; CHECK: call i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16> [[DOWNCAST]], i32 2, i32 1, i32 2, i16 0, i32 undef)
  %1 = call i16  @llvm.cttz.i16(i16 %arg, i1 false)

; CHECK: [[ZEXT_1:%.*]] = zext i16 %arg to i32
; CHECK: [[CTLZ:%.*]] = call i32 @llvm.ctlz.i32(i32 [[ZEXT_1]], i1 false)
; CHECK: [[SUB:%.*]] = sub i32 [[CTLZ]], 16
; CHECK: [[DOWNCAST:%.*]] = bitcast i32 [[SUB]] to <2 x i16>
; CHECK: call i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16> [[DOWNCAST]], i32 2, i32 1, i32 2, i16 0, i32 undef)

  %2 = call i16  @llvm.ctlz.i16(i16 %arg, i1 false)
  ret void
}
