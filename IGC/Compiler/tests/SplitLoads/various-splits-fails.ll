;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; REQUIRES: regkeys
; RUN: igc_opt -S --igc-split-loads -platformpvc --regkey=LS_enableLoadSplitting=1 --regkey=LS_ignoreSplitThreshold=1 --regkey=LS_minSplitSize_GRF=0 --regkey=LS_minSplitSize_E=0 %s | FileCheck %s --check-prefix=MINSPLIT
; RUN: igc_opt -S --igc-split-loads -platformpvc --regkey=LS_enableLoadSplitting=1 --regkey=LS_ignoreSplitThreshold=1 --regkey=LS_minSplitSize_GRF=0 --regkey=LS_minSplitSize_E=8 %s | FileCheck %s --check-prefix=SPLIT8


declare spir_func <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare spir_func <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare spir_func <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare spir_func <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare spir_func void @fun_v2fl(<2 x float>)
declare spir_func void @fun_v4fl(<4 x float>)
declare spir_func void @fun_v8fl(<8 x float>)

declare spir_func void @fun_v2i32(<2 x i32>)
declare spir_func void @fun_v4i32(<4 x i32>)
declare spir_func void @fun_v8i32(<8 x i32>)
declare spir_func void @fun_v16i32(<16 x i32>)

declare spir_func void @fun_v2i16(<2 x i16>)
declare spir_func void @fun_v4i16(<4 x i16>)
declare spir_func void @fun_v8i16(<8 x i16>)

define spir_kernel void @test_basic_splits_fail_1(i64 %ptr) {
; MINSPLIT-LABEL: @test_basic_splits_fail_1(
; MINSPLIT-NEXT:    [[VEC1:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; MINSPLIT-NEXT:    [[PICK1_1:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; MINSPLIT-NEXT:    [[PICK1_2:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
; MINSPLIT-NEXT:    [[PICK1_3:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; MINSPLIT-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_1]])
; MINSPLIT-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_2]])
; MINSPLIT-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1_3]])
; MINSPLIT-NEXT:    ret void
;
; SPLIT8-LABEL: @test_basic_splits_fail_1(
; SPLIT8-NEXT:    [[VEC1:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; SPLIT8-NEXT:    [[PICK1_1:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; SPLIT8-NEXT:    [[PICK1_2:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
; SPLIT8-NEXT:    [[PICK1_3:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; SPLIT8-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_1]])
; SPLIT8-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_2]])
; SPLIT8-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1_3]])
; SPLIT8-NEXT:    ret void
;
  %vec1 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr, i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick1.1 = shufflevector <16 x i16> %vec1, <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %pick1.2 = shufflevector <16 x i16> %vec1, <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  %pick1.3 = shufflevector <16 x i16> %vec1, <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v4i16(<4 x i16> %pick1.1)
  call void @fun_v4i16(<4 x i16> %pick1.2)
  call void @fun_v8i16(<8 x i16> %pick1.3)
  ret void
}

define spir_kernel void @test_basic_splits_fail_4(i64 %ptr) {
; MINSPLIT-LABEL: @test_basic_splits_fail_4(
; MINSPLIT-NEXT:    [[VEC1B:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; MINSPLIT-NEXT:    [[PICK1B_1:%.*]] = shufflevector <16 x i16> [[VEC1B]], <16 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 8, i32 9, i32 10, i32 11>
; MINSPLIT-NEXT:    [[PICK1B_2:%.*]] = shufflevector <16 x i16> [[VEC1B]], <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 12, i32 13, i32 14, i32 15>
; MINSPLIT-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1B_1]])
; MINSPLIT-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1B_2]])
; MINSPLIT-NEXT:    ret void
;
; SPLIT8-LABEL: @test_basic_splits_fail_4(
; SPLIT8-NEXT:    [[VEC1B:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; SPLIT8-NEXT:    [[PICK1B_1:%.*]] = shufflevector <16 x i16> [[VEC1B]], <16 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 8, i32 9, i32 10, i32 11>
; SPLIT8-NEXT:    [[PICK1B_2:%.*]] = shufflevector <16 x i16> [[VEC1B]], <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 12, i32 13, i32 14, i32 15>
; SPLIT8-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1B_1]])
; SPLIT8-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1B_2]])
; SPLIT8-NEXT:    ret void
;
  %vec1b = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr, i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick1b.1 = shufflevector <16 x i16> %vec1b, <16 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 8, i32 9, i32 10, i32 11>
  %pick1b.2 = shufflevector <16 x i16> %vec1b, <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i16(<8 x i16> %pick1b.1)
  call void @fun_v8i16(<8 x i16> %pick1b.2)
  ret void
}

define spir_kernel void @test_basic_splits_fail_5(i64 %ptr) {
; MINSPLIT-LABEL: @test_basic_splits_fail_5(
; MINSPLIT-NEXT:    [[VEC4:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 31, i32 63, i32 127, i32 8, i32 4, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; MINSPLIT-NEXT:    [[PICK4_1:%.*]] = shufflevector <16 x i16> [[VEC4]], <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11>
; MINSPLIT-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK4_1]])
; MINSPLIT-NEXT:    ret void
;
; SPLIT8-LABEL: @test_basic_splits_fail_5(
; SPLIT8-NEXT:    [[VEC4:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 31, i32 63, i32 127, i32 8, i32 4, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; SPLIT8-NEXT:    [[PICK4_1:%.*]] = shufflevector <16 x i16> [[VEC4]], <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11>
; SPLIT8-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK4_1]])
; SPLIT8-NEXT:    ret void
;
  %vec4 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr, i32 31, i32 63, i32 127, i32 8, i32 4, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick4.1 = shufflevector <16 x i16> %vec4, <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11>
  call void @fun_v8i16(<8 x i16> %pick4.1)
  ret void
}

define spir_kernel void @test_basic_block_splits_fail_1(i64 %ptr) {
; MINSPLIT-LABEL: @test_basic_block_splits_fail_1(
; MINSPLIT-NEXT:    [[VEC1:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
; MINSPLIT-NEXT:    [[PICK1_1:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; MINSPLIT-NEXT:    [[PICK1_2:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
; MINSPLIT-NEXT:    [[PICK1_3:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; MINSPLIT-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_1]])
; MINSPLIT-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_2]])
; MINSPLIT-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1_3]])
; MINSPLIT-NEXT:    ret void
;
; SPLIT8-LABEL: @test_basic_block_splits_fail_1(
; SPLIT8-NEXT:    [[VEC1:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
; SPLIT8-NEXT:    [[PICK1_1:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; SPLIT8-NEXT:    [[PICK1_2:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
; SPLIT8-NEXT:    [[PICK1_3:%.*]] = shufflevector <16 x i16> [[VEC1]], <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; SPLIT8-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_1]])
; SPLIT8-NEXT:    call void @fun_v4i16(<4 x i16> [[PICK1_2]])
; SPLIT8-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK1_3]])
; SPLIT8-NEXT:    ret void
;
  %vec1 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr, i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  %pick1.1 = shufflevector <16 x i16> %vec1, <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %pick1.2 = shufflevector <16 x i16> %vec1, <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  %pick1.3 = shufflevector <16 x i16> %vec1, <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v4i16(<4 x i16> %pick1.1)
  call void @fun_v4i16(<4 x i16> %pick1.2)
  call void @fun_v8i16(<8 x i16> %pick1.3)
  ret void
}

define spir_kernel void @test_basic_block_splits_fail_5(i64 %ptr) {
; MINSPLIT-LABEL: @test_basic_block_splits_fail_5(
; MINSPLIT-NEXT:    [[VEC4:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 31, i32 63, i32 127, i32 8, i32 4, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
; MINSPLIT-NEXT:    [[PICK4_1:%.*]] = shufflevector <16 x i16> [[VEC4]], <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11>
; MINSPLIT-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK4_1]])
; MINSPLIT-NEXT:    ret void
;
; SPLIT8-LABEL: @test_basic_block_splits_fail_5(
; SPLIT8-NEXT:    [[VEC4:%.*]] = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 [[PTR:%.*]], i32 31, i32 63, i32 127, i32 8, i32 4, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
; SPLIT8-NEXT:    [[PICK4_1:%.*]] = shufflevector <16 x i16> [[VEC4]], <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11>
; SPLIT8-NEXT:    call void @fun_v8i16(<8 x i16> [[PICK4_1]])
; SPLIT8-NEXT:    ret void
;
  %vec4 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr, i32 31, i32 63, i32 127, i32 8, i32 4, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  %pick4.1 = shufflevector <16 x i16> %vec4, <16 x i16> undef, <8 x i32> <i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11>
  call void @fun_v8i16(<8 x i16> %pick4.1)
  ret void
}

define spir_kernel void @test_transposed_fails(i64 %ptr) {
; MINSPLIT-LABEL: @test_transposed_fails(
; MINSPLIT-NEXT:    [[VEC1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 63, i32 63, i32 63, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
; MINSPLIT-NEXT:    [[PICK1_1:%.*]] = shufflevector <8 x i32> [[VEC1]], <8 x i32> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
; MINSPLIT-NEXT:    [[PICK1_2:%.*]] = shufflevector <8 x i32> [[VEC1]], <8 x i32> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
; MINSPLIT-NEXT:    call void @fun_v4i32(<4 x i32> [[PICK1_1]])
; MINSPLIT-NEXT:    call void @fun_v4i32(<4 x i32> [[PICK1_2]])
; MINSPLIT-NEXT:    ret void
;
; SPLIT8-LABEL: @test_transposed_fails(
; SPLIT8-NEXT:    [[VEC1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 63, i32 63, i32 63, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
; SPLIT8-NEXT:    [[PICK1_1:%.*]] = shufflevector <8 x i32> [[VEC1]], <8 x i32> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
; SPLIT8-NEXT:    [[PICK1_2:%.*]] = shufflevector <8 x i32> [[VEC1]], <8 x i32> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
; SPLIT8-NEXT:    call void @fun_v4i32(<4 x i32> [[PICK1_1]])
; SPLIT8-NEXT:    call void @fun_v4i32(<4 x i32> [[PICK1_2]])
; SPLIT8-NEXT:    ret void
;
  %vec1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %ptr, i32 63, i32 63, i32 63, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %pick1.1 = shufflevector <8 x i32> %vec1, <8 x i32> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %pick1.2 = shufflevector <8 x i32> %vec1, <8 x i32> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  call void @fun_v4i32(<4 x i32> %pick1.1)
  call void @fun_v4i32(<4 x i32> %pick1.2)

  ret void
}
