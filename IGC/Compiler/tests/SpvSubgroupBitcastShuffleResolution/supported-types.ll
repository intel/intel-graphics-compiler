;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-spv-subgroup-bitcast-shuffle-resolution -S < %s 2>&1 | FileCheck %s --implicit-check-not=error: --implicit-check-not="call spir_func"

; Each row of the Supported Types table is exercised in both directions.
; https://github.com/KhronosGroup/SPIRV-Registry/pull/454

target triple = "spir64-unknown-unknown"

define <2 x i8> @test_v2i8_i16(<2 x i8> %src) {
; CHECK-LABEL: define <2 x i8> @test_v2i8_i16(
; CHECK: [[FORWARD:%.*]] = call i16 @llvm.genx.GenISA.SubgroupBitcastShuffle.i16.v2i8(<2 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <2 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i8.i16(i16 [[FORWARD]])
; CHECK-NEXT: ret <2 x i8> [[BACKWARD]]
  %forward = call spir_func i16 @__spirv_SubgroupBitcastShuffleINTEL_v2i8_to_i16(<2 x i8> %src)
  %backward = call spir_func <2 x i8> @__spirv_SubgroupBitcastShuffleINTEL_i16_to_v2i8(i16 %forward)
  ret <2 x i8> %backward
}

declare spir_func i16 @__spirv_SubgroupBitcastShuffleINTEL_v2i8_to_i16(<2 x i8>)
declare spir_func <2 x i8> @__spirv_SubgroupBitcastShuffleINTEL_i16_to_v2i8(i16)

define <4 x i8> @test_v4i8_v2i16(<4 x i8> %src) {
; CHECK-LABEL: define <4 x i8> @test_v4i8_v2i16(
; CHECK: [[FORWARD:%.*]] = call <2 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i16.v4i8(<4 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <4 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i8.v2i16(<2 x i16> [[FORWARD]])
; CHECK-NEXT: ret <4 x i8> [[BACKWARD]]
  %forward = call spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v4i8_to_v2i16(<4 x i8> %src)
  %backward = call spir_func <4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v2i16_to_v4i8(<2 x i16> %forward)
  ret <4 x i8> %backward
}

declare spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v4i8_to_v2i16(<4 x i8>)
declare spir_func <4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v2i16_to_v4i8(<2 x i16>)

define <4 x i8> @test_v4i8_i32(<4 x i8> %src) {
; CHECK-LABEL: define <4 x i8> @test_v4i8_i32(
; CHECK: [[FORWARD:%.*]] = call i32 @llvm.genx.GenISA.SubgroupBitcastShuffle.i32.v4i8(<4 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <4 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i8.i32(i32 [[FORWARD]])
; CHECK-NEXT: ret <4 x i8> [[BACKWARD]]
  %forward = call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_v4i8_to_i32(<4 x i8> %src)
  %backward = call spir_func <4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_i32_to_v4i8(i32 %forward)
  ret <4 x i8> %backward
}

declare spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_v4i8_to_i32(<4 x i8>)
declare spir_func <4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_i32_to_v4i8(i32)

define <2 x i16> @test_v2i16_i32(<2 x i16> %src) {
; CHECK-LABEL: define <2 x i16> @test_v2i16_i32(
; CHECK: [[FORWARD:%.*]] = call i32 @llvm.genx.GenISA.SubgroupBitcastShuffle.i32.v2i16(<2 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <2 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i16.i32(i32 [[FORWARD]])
; CHECK-NEXT: ret <2 x i16> [[BACKWARD]]
  %forward = call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_v2i16_to_i32(<2 x i16> %src)
  %backward = call spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_i32_to_v2i16(i32 %forward)
  ret <2 x i16> %backward
}

declare spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_v2i16_to_i32(<2 x i16>)
declare spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_i32_to_v2i16(i32)

define <8 x i8> @test_v8i8_v4i16(<8 x i8> %src) {
; CHECK-LABEL: define <8 x i8> @test_v8i8_v4i16(
; CHECK: [[FORWARD:%.*]] = call <4 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i16.v8i8(<8 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <8 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i8.v4i16(<4 x i16> [[FORWARD]])
; CHECK-NEXT: ret <8 x i8> [[BACKWARD]]
  %forward = call spir_func <4 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v8i8_to_v4i16(<8 x i8> %src)
  %backward = call spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v4i16_to_v8i8(<4 x i16> %forward)
  ret <8 x i8> %backward
}

declare spir_func <4 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v8i8_to_v4i16(<8 x i8>)
declare spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v4i16_to_v8i8(<4 x i16>)

define <8 x i8> @test_v8i8_v2i32(<8 x i8> %src) {
; CHECK-LABEL: define <8 x i8> @test_v8i8_v2i32(
; CHECK: [[FORWARD:%.*]] = call <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.v8i8(<8 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <8 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i8.v2i32(<2 x i32> [[FORWARD]])
; CHECK-NEXT: ret <8 x i8> [[BACKWARD]]
  %forward = call spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v8i8_to_v2i32(<8 x i8> %src)
  %backward = call spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v2i32_to_v8i8(<2 x i32> %forward)
  ret <8 x i8> %backward
}

declare spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v8i8_to_v2i32(<8 x i8>)
declare spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v2i32_to_v8i8(<2 x i32>)

define <8 x i8> @test_v8i8_i64(<8 x i8> %src) {
; CHECK-LABEL: define <8 x i8> @test_v8i8_i64(
; CHECK: [[FORWARD:%.*]] = call i64 @llvm.genx.GenISA.SubgroupBitcastShuffle.i64.v8i8(<8 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <8 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i8.i64(i64 [[FORWARD]])
; CHECK-NEXT: ret <8 x i8> [[BACKWARD]]
  %forward = call spir_func i64 @__spirv_SubgroupBitcastShuffleINTEL_v8i8_to_i64(<8 x i8> %src)
  %backward = call spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_i64_to_v8i8(i64 %forward)
  ret <8 x i8> %backward
}

declare spir_func i64 @__spirv_SubgroupBitcastShuffleINTEL_v8i8_to_i64(<8 x i8>)
declare spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_i64_to_v8i8(i64)

define <4 x i16> @test_v4i16_v2i32(<4 x i16> %src) {
; CHECK-LABEL: define <4 x i16> @test_v4i16_v2i32(
; CHECK: [[FORWARD:%.*]] = call <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.v4i16(<4 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <4 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i16.v2i32(<2 x i32> [[FORWARD]])
; CHECK-NEXT: ret <4 x i16> [[BACKWARD]]
  %forward = call spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v4i16_to_v2i32(<4 x i16> %src)
  %backward = call spir_func <4 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v2i32_to_v4i16(<2 x i32> %forward)
  ret <4 x i16> %backward
}

declare spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v4i16_to_v2i32(<4 x i16>)
declare spir_func <4 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v2i32_to_v4i16(<2 x i32>)

define <4 x i16> @test_v4i16_i64(<4 x i16> %src) {
; CHECK-LABEL: define <4 x i16> @test_v4i16_i64(
; CHECK: [[FORWARD:%.*]] = call i64 @llvm.genx.GenISA.SubgroupBitcastShuffle.i64.v4i16(<4 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <4 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i16.i64(i64 [[FORWARD]])
; CHECK-NEXT: ret <4 x i16> [[BACKWARD]]
  %forward = call spir_func i64 @__spirv_SubgroupBitcastShuffleINTEL_v4i16_to_i64(<4 x i16> %src)
  %backward = call spir_func <4 x i16> @__spirv_SubgroupBitcastShuffleINTEL_i64_to_v4i16(i64 %forward)
  ret <4 x i16> %backward
}

declare spir_func i64 @__spirv_SubgroupBitcastShuffleINTEL_v4i16_to_i64(<4 x i16>)
declare spir_func <4 x i16> @__spirv_SubgroupBitcastShuffleINTEL_i64_to_v4i16(i64)

define <2 x i32> @test_v2i32_i64(<2 x i32> %src) {
; CHECK-LABEL: define <2 x i32> @test_v2i32_i64(
; CHECK: [[FORWARD:%.*]] = call i64 @llvm.genx.GenISA.SubgroupBitcastShuffle.i64.v2i32(<2 x i32> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.i64(i64 [[FORWARD]])
; CHECK-NEXT: ret <2 x i32> [[BACKWARD]]
  %forward = call spir_func i64 @__spirv_SubgroupBitcastShuffleINTEL_v2i32_to_i64(<2 x i32> %src)
  %backward = call spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_i64_to_v2i32(i64 %forward)
  ret <2 x i32> %backward
}

declare spir_func i64 @__spirv_SubgroupBitcastShuffleINTEL_v2i32_to_i64(<2 x i32>)
declare spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_i64_to_v2i32(i64)

define <16 x i8> @test_v16i8_v8i16(<16 x i8> %src) {
; CHECK-LABEL: define <16 x i8> @test_v16i8_v8i16(
; CHECK: [[FORWARD:%.*]] = call <8 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i16.v16i8(<16 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <16 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v16i8.v8i16(<8 x i16> [[FORWARD]])
; CHECK-NEXT: ret <16 x i8> [[BACKWARD]]
  %forward = call spir_func <8 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v16i8_to_v8i16(<16 x i8> %src)
  %backward = call spir_func <16 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v8i16_to_v16i8(<8 x i16> %forward)
  ret <16 x i8> %backward
}

declare spir_func <8 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v16i8_to_v8i16(<16 x i8>)
declare spir_func <16 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v8i16_to_v16i8(<8 x i16>)

define <16 x i8> @test_v16i8_v4i32(<16 x i8> %src) {
; CHECK-LABEL: define <16 x i8> @test_v16i8_v4i32(
; CHECK: [[FORWARD:%.*]] = call <4 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i32.v16i8(<16 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <16 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v16i8.v4i32(<4 x i32> [[FORWARD]])
; CHECK-NEXT: ret <16 x i8> [[BACKWARD]]
  %forward = call spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v16i8_to_v4i32(<16 x i8> %src)
  %backward = call spir_func <16 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v4i32_to_v16i8(<4 x i32> %forward)
  ret <16 x i8> %backward
}

declare spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v16i8_to_v4i32(<16 x i8>)
declare spir_func <16 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v4i32_to_v16i8(<4 x i32>)

define <16 x i8> @test_v16i8_v2i64(<16 x i8> %src) {
; CHECK-LABEL: define <16 x i8> @test_v16i8_v2i64(
; CHECK: [[FORWARD:%.*]] = call <2 x i64> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i64.v16i8(<16 x i8> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <16 x i8> @llvm.genx.GenISA.SubgroupBitcastShuffle.v16i8.v2i64(<2 x i64> [[FORWARD]])
; CHECK-NEXT: ret <16 x i8> [[BACKWARD]]
  %forward = call spir_func <2 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v16i8_to_v2i64(<16 x i8> %src)
  %backward = call spir_func <16 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v2i64_to_v16i8(<2 x i64> %forward)
  ret <16 x i8> %backward
}

declare spir_func <2 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v16i8_to_v2i64(<16 x i8>)
declare spir_func <16 x i8> @__spirv_SubgroupBitcastShuffleINTEL_v2i64_to_v16i8(<2 x i64>)

define <8 x i16> @test_v8i16_v4i32(<8 x i16> %src) {
; CHECK-LABEL: define <8 x i16> @test_v8i16_v4i32(
; CHECK: [[FORWARD:%.*]] = call <4 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i32.v8i16(<8 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <8 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i16.v4i32(<4 x i32> [[FORWARD]])
; CHECK-NEXT: ret <8 x i16> [[BACKWARD]]
  %forward = call spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v8i16_to_v4i32(<8 x i16> %src)
  %backward = call spir_func <8 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v4i32_to_v8i16(<4 x i32> %forward)
  ret <8 x i16> %backward
}

declare spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v8i16_to_v4i32(<8 x i16>)
declare spir_func <8 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v4i32_to_v8i16(<4 x i32>)

define <8 x i16> @test_v8i16_v2i64(<8 x i16> %src) {
; CHECK-LABEL: define <8 x i16> @test_v8i16_v2i64(
; CHECK: [[FORWARD:%.*]] = call <2 x i64> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i64.v8i16(<8 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <8 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i16.v2i64(<2 x i64> [[FORWARD]])
; CHECK-NEXT: ret <8 x i16> [[BACKWARD]]
  %forward = call spir_func <2 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v8i16_to_v2i64(<8 x i16> %src)
  %backward = call spir_func <8 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v2i64_to_v8i16(<2 x i64> %forward)
  ret <8 x i16> %backward
}

declare spir_func <2 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v8i16_to_v2i64(<8 x i16>)
declare spir_func <8 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v2i64_to_v8i16(<2 x i64>)

define <4 x i32> @test_v4i32_v2i64(<4 x i32> %src) {
; CHECK-LABEL: define <4 x i32> @test_v4i32_v2i64(
; CHECK: [[FORWARD:%.*]] = call <2 x i64> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i64.v4i32(<4 x i32> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <4 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i32.v2i64(<2 x i64> [[FORWARD]])
; CHECK-NEXT: ret <4 x i32> [[BACKWARD]]
  %forward = call spir_func <2 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v4i32_to_v2i64(<4 x i32> %src)
  %backward = call spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v2i64_to_v4i32(<2 x i64> %forward)
  ret <4 x i32> %backward
}

declare spir_func <2 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v4i32_to_v2i64(<4 x i32>)
declare spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v2i64_to_v4i32(<2 x i64>)

define <16 x i16> @test_v16i16_v8i32(<16 x i16> %src) {
; CHECK-LABEL: define <16 x i16> @test_v16i16_v8i32(
; CHECK: [[FORWARD:%.*]] = call <8 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i32.v16i16(<16 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <16 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v16i16.v8i32(<8 x i32> [[FORWARD]])
; CHECK-NEXT: ret <16 x i16> [[BACKWARD]]
  %forward = call spir_func <8 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v16i16_to_v8i32(<16 x i16> %src)
  %backward = call spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v8i32_to_v16i16(<8 x i32> %forward)
  ret <16 x i16> %backward
}

declare spir_func <8 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v16i16_to_v8i32(<16 x i16>)
declare spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v8i32_to_v16i16(<8 x i32>)

define <16 x i16> @test_v16i16_v4i64(<16 x i16> %src) {
; CHECK-LABEL: define <16 x i16> @test_v16i16_v4i64(
; CHECK: [[FORWARD:%.*]] = call <4 x i64> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i64.v16i16(<16 x i16> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <16 x i16> @llvm.genx.GenISA.SubgroupBitcastShuffle.v16i16.v4i64(<4 x i64> [[FORWARD]])
; CHECK-NEXT: ret <16 x i16> [[BACKWARD]]
  %forward = call spir_func <4 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v16i16_to_v4i64(<16 x i16> %src)
  %backward = call spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v4i64_to_v16i16(<4 x i64> %forward)
  ret <16 x i16> %backward
}

declare spir_func <4 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v16i16_to_v4i64(<16 x i16>)
declare spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_v4i64_to_v16i16(<4 x i64>)

define <8 x i32> @test_v8i32_v4i64(<8 x i32> %src) {
; CHECK-LABEL: define <8 x i32> @test_v8i32_v4i64(
; CHECK: [[FORWARD:%.*]] = call <4 x i64> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i64.v8i32(<8 x i32> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <8 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i32.v4i64(<4 x i64> [[FORWARD]])
; CHECK-NEXT: ret <8 x i32> [[BACKWARD]]
  %forward = call spir_func <4 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v8i32_to_v4i64(<8 x i32> %src)
  %backward = call spir_func <8 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v4i64_to_v8i32(<4 x i64> %forward)
  ret <8 x i32> %backward
}

declare spir_func <4 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v8i32_to_v4i64(<8 x i32>)
declare spir_func <8 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v4i64_to_v8i32(<4 x i64>)

define <16 x i32> @test_v16i32_v8i64(<16 x i32> %src) {
; CHECK-LABEL: define <16 x i32> @test_v16i32_v8i64(
; CHECK: [[FORWARD:%.*]] = call <8 x i64> @llvm.genx.GenISA.SubgroupBitcastShuffle.v8i64.v16i32(<16 x i32> %src)
; CHECK-NEXT: [[BACKWARD:%.*]] = call <16 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v16i32.v8i64(<8 x i64> [[FORWARD]])
; CHECK-NEXT: ret <16 x i32> [[BACKWARD]]
  %forward = call spir_func <8 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v16i32_to_v8i64(<16 x i32> %src)
  %backward = call spir_func <16 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v8i64_to_v16i32(<8 x i64> %forward)
  ret <16 x i32> %backward
}

declare spir_func <8 x i64> @__spirv_SubgroupBitcastShuffleINTEL_v16i32_to_v8i64(<16 x i32>)
declare spir_func <16 x i32> @__spirv_SubgroupBitcastShuffleINTEL_v8i64_to_v16i32(<8 x i64>)

