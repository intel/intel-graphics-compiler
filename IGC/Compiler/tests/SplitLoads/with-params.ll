;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; REQUIRES: regkeys
; RUN: igc_opt -S --igc-split-loads -platformpvc --regkey=LS_enableLoadSplitting=1 --regkey=LS_ignoreSplitThreshold=1 --regkey=LS_minSplitSize_GRF=0 --regkey=LS_minSplitSize_E=0 %s | FileCheck %s

declare spir_func <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare spir_func <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare spir_func void @fun_v2fl(<2 x float>)
declare spir_func void @fun_v4fl(<4 x float>)
declare spir_func void @fun_v8fl(<8 x float>)

declare spir_func void @fun_v2i32(<2 x i32>)
declare spir_func void @fun_v4i32(<4 x i32>)
declare spir_func void @fun_v8i32(<8 x i32>)

declare spir_func void @fun_v2i16(<2 x i16>)
declare spir_func void @fun_v4i16(<4 x i16>)
declare spir_func void @fun_v8i16(<8 x i16>)

define spir_kernel void @test_consecutive_splits_with_offsets_1(i64 %ptr) {
; CHECK-LABEL: @test_consecutive_splits_with_offsets_1(
; CHECK-NEXT:    [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 16, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR]], i32 127, i32 63, i32 127, i32 16, i32 8, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP2]])
; CHECK-NEXT:    ret void
;
  %vec1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 127, i32 63, i32 127, i32 16, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick1.1 = shufflevector <16 x i32> %vec1, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %pick1.2 = shufflevector <16 x i32> %vec1, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i32(<8 x i32> %pick1.1)
  call void @fun_v8i32(<8 x i32> %pick1.2)
  ret void
}

define spir_kernel void @test_consecutive_splits_with_offsets_2(i64 %ptr) {
; CHECK-LABEL: @test_consecutive_splits_with_offsets_2(
; CHECK-NEXT:    [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 0, i32 4, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR]], i32 127, i32 63, i32 127, i32 0, i32 12, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP2]])
; CHECK-NEXT:    ret void
;
  %vec2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 127, i32 63, i32 127, i32 0, i32 4, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick2.1 = shufflevector <16 x i32> %vec2, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %pick2.2 = shufflevector <16 x i32> %vec2, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i32(<8 x i32> %pick2.1)
  call void @fun_v8i32(<8 x i32> %pick2.2)
  ret void
}

define spir_kernel void @test_consecutive_splits_with_offsets_3(i64 %ptr) {
; CHECK-LABEL: @test_consecutive_splits_with_offsets_3(
; CHECK-NEXT:    [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 4, i32 8, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR]], i32 127, i32 63, i32 127, i32 4, i32 16, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP2]])
; CHECK-NEXT:    ret void
;
  %vec3 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 127, i32 63, i32 127, i32 4, i32 8, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick3.1 = shufflevector <16 x i32> %vec3, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %pick3.2 = shufflevector <16 x i32> %vec3, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i32(<8 x i32> %pick3.1)
  call void @fun_v8i32(<8 x i32> %pick3.2)
  ret void
}

define spir_kernel void @test_consecutive_splits_with_offsets_4(i64 %ptr) {
; CHECK-LABEL: @test_consecutive_splits_with_offsets_4(
; CHECK-NEXT:    [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 64, i32 8, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR]], i32 127, i32 63, i32 127, i32 64, i32 16, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP2]])
; CHECK-NEXT:    ret void
;
  %vec4 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 127, i32 63, i32 127, i32 64, i32 8, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick4.1 = shufflevector <16 x i32> %vec4, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %pick4.2 = shufflevector <16 x i32> %vec4, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i32(<8 x i32> %pick4.1)
  call void @fun_v8i32(<8 x i32> %pick4.2)
  ret void
}

define spir_kernel void @test_strided_splits_with_width_1(i64 %ptr) {
; CHECK-LABEL: @test_strided_splits_with_width_1(
; CHECK-NEXT:    [[VEC1:%.*]] = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 [[PTR:%.*]], i32 63, i32 63, i32 63, i32 0, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[PICK1_1:%.*]] = shufflevector <16 x i32> [[VEC1]], <16 x i32> undef, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
; CHECK-NEXT:    [[PICK1_2:%.*]] = shufflevector <16 x i32> [[VEC1]], <16 x i32> undef, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[PICK1_1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[PICK1_2]])
; CHECK-NEXT:    ret void
;
  %vec1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 63, i32 63, i32 63, i32 0, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick1.1 = shufflevector <16 x i32> %vec1, <16 x i32> undef, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
  %pick1.2 = shufflevector <16 x i32> %vec1, <16 x i32> undef, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  call void @fun_v8i32(<8 x i32> %pick1.1)
  call void @fun_v8i32(<8 x i32> %pick1.2)
  ret void
}

define spir_kernel void @test_non_constant_parameters_1(i64 %ptr, i32 %x, i32 %y) {
; CHECK-LABEL: @test_non_constant_parameters_1(
; CHECK-NEXT:    [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 [[X:%.*]], i32 [[Y:%.*]], i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i32 [[Y]], 8
; CHECK-NEXT:    [[TMP3:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR]], i32 127, i32 63, i32 127, i32 [[X]], i32 [[TMP2]], i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP3]])
; CHECK-NEXT:    ret void
;
  %vec1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 127, i32 63, i32 127, i32 %x, i32 %y, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick1.1 = shufflevector <16 x i32> %vec1, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %pick1.2 = shufflevector <16 x i32> %vec1, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i32(<8 x i32> %pick1.1)
  call void @fun_v8i32(<8 x i32> %pick1.2)
  ret void
}

define spir_kernel void @test_non_constant_parameters_2(i64 %ptr, i32 %x, i32 %y) {
; CHECK-LABEL: @test_non_constant_parameters_2(
; CHECK-NEXT:    [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 [[X:%.*]], i32 [[Y:%.*]], i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i32 [[Y]], 8
; CHECK-NEXT:    [[TMP3:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[PTR]], i32 127, i32 63, i32 127, i32 [[X]], i32 [[TMP2]], i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP1]])
; CHECK-NEXT:    call void @fun_v8i32(<8 x i32> [[TMP3]])
; CHECK-NEXT:    ret void
;
  %vec2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v16i32(i64 %ptr, i32 127, i32 63, i32 127, i32 %x, i32 %y, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %pick2.1 = shufflevector <16 x i32> %vec2, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %pick2.2 = shufflevector <16 x i32> %vec2, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call void @fun_v8i32(<8 x i32> %pick2.1)
  call void @fun_v8i32(<8 x i32> %pick2.2)
  ret void
}

define spir_kernel void @test_non_constant_parameters_3(i64 %ptr, i32 %x, i32 %y) {
; CHECK-LABEL: @test_non_constant_parameters_3(
; CHECK-NEXT:    [[TMP1:%.*]] = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 [[PTR:%.*]], i32 127, i32 63, i32 127, i32 [[X:%.*]], i32 [[Y:%.*]], i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i32 [[Y]], 4
; CHECK-NEXT:    [[TMP3:%.*]] = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 [[PTR]], i32 127, i32 63, i32 127, i32 [[X]], i32 [[TMP2]], i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP4:%.*]] = add nuw nsw i32 [[X]], 16
; CHECK-NEXT:    [[TMP5:%.*]] = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 [[PTR]], i32 127, i32 63, i32 127, i32 [[TMP4]], i32 [[Y]], i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    [[TMP6:%.*]] = add nuw nsw i32 [[X]], 16
; CHECK-NEXT:    [[TMP7:%.*]] = add nuw nsw i32 [[Y]], 4
; CHECK-NEXT:    [[TMP8:%.*]] = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 [[PTR]], i32 127, i32 63, i32 127, i32 [[TMP6]], i32 [[TMP7]], i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:    call void @fun_v4i16(<4 x i16> [[TMP1]])
; CHECK-NEXT:    call void @fun_v4i16(<4 x i16> [[TMP3]])
; CHECK-NEXT:    call void @fun_v4i16(<4 x i16> [[TMP5]])
; CHECK-NEXT:    call void @fun_v4i16(<4 x i16> [[TMP8]])
; CHECK-NEXT:    ret void
;
  %vec3 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr, i32 127, i32 63, i32 127, i32 %x, i32 %y, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  %pick3.1 = shufflevector <16 x i16> %vec3, <16 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %pick3.2 = shufflevector <16 x i16> %vec3, <16 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  %pick3.3 = shufflevector <16 x i16> %vec3, <16 x i16> undef, <4 x i32> <i32 8, i32 9, i32 10, i32 11>
  %pick3.4 = shufflevector <16 x i16> %vec3, <16 x i16> undef, <4 x i32> <i32 12, i32 13, i32 14, i32 15>
  call void @fun_v4i16(<4 x i16> %pick3.1)
  call void @fun_v4i16(<4 x i16> %pick3.2)
  call void @fun_v4i16(<4 x i16> %pick3.3)
  call void @fun_v4i16(<4 x i16> %pick3.4)
  ret void
}
