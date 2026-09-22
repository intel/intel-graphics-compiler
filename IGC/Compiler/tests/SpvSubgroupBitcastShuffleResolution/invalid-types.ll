;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-spv-subgroup-bitcast-shuffle-resolution -S < %s > %t.out 2> %t.err
; RUN: FileCheck %s --check-prefix=IR --input-file=%t.out --implicit-check-not=llvm.genx.GenISA.SubgroupBitcastShuffle
; RUN: FileCheck %s --check-prefix=ERROR --input-file=%t.err

; Equal total size alone does not make a type pair valid. Invalid calls must
; retain their original form and produce a diagnostic instead of an intrinsic.

target triple = "spir64-unknown-unknown"

define spir_kernel void @test_half_result() {
; IR-LABEL: define spir_kernel void @test_half_result(
; IR: call spir_func half @__spirv_SubgroupBitcastShuffleINTEL_half_result(<2 x i8> undef)
; ERROR: error: in function 'test_half_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func half @__spirv_SubgroupBitcastShuffleINTEL_half_result(<2 x i8> undef)
  ret void
}

declare spir_func half @__spirv_SubgroupBitcastShuffleINTEL_half_result(<2 x i8>)

define spir_kernel void @test_half_operand() {
; IR-LABEL: define spir_kernel void @test_half_operand(
; IR: call spir_func <2 x i8> @__spirv_SubgroupBitcastShuffleINTEL_half_operand(half undef)
; ERROR: error: in function 'test_half_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <2 x i8> @__spirv_SubgroupBitcastShuffleINTEL_half_operand(half undef)
  ret void
}

declare spir_func <2 x i8> @__spirv_SubgroupBitcastShuffleINTEL_half_operand(half)

define spir_kernel void @test_float_result() {
; IR-LABEL: define spir_kernel void @test_float_result(
; IR: call spir_func float @__spirv_SubgroupBitcastShuffleINTEL_float_result(<2 x i16> undef)
; ERROR: error: in function 'test_float_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func float @__spirv_SubgroupBitcastShuffleINTEL_float_result(<2 x i16> undef)
  ret void
}

declare spir_func float @__spirv_SubgroupBitcastShuffleINTEL_float_result(<2 x i16>)

define spir_kernel void @test_float_operand() {
; IR-LABEL: define spir_kernel void @test_float_operand(
; IR: call spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_float_operand(float undef)
; ERROR: error: in function 'test_float_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_float_operand(float undef)
  ret void
}

declare spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_float_operand(float)

define spir_kernel void @test_double_result() {
; IR-LABEL: define spir_kernel void @test_double_result(
; IR: call spir_func double @__spirv_SubgroupBitcastShuffleINTEL_double_result(<2 x i32> undef)
; ERROR: error: in function 'test_double_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func double @__spirv_SubgroupBitcastShuffleINTEL_double_result(<2 x i32> undef)
  ret void
}

declare spir_func double @__spirv_SubgroupBitcastShuffleINTEL_double_result(<2 x i32>)

define spir_kernel void @test_double_operand() {
; IR-LABEL: define spir_kernel void @test_double_operand(
; IR: call spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_double_operand(double undef)
; ERROR: error: in function 'test_double_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_double_operand(double undef)
  ret void
}

declare spir_func <2 x i32> @__spirv_SubgroupBitcastShuffleINTEL_double_operand(double)

define spir_kernel void @test_float_vector_result() {
; IR-LABEL: define spir_kernel void @test_float_vector_result(
; IR: call spir_func <2 x float> @__spirv_SubgroupBitcastShuffleINTEL_float_vector_result(<8 x i8> undef)
; ERROR: error: in function 'test_float_vector_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <2 x float> @__spirv_SubgroupBitcastShuffleINTEL_float_vector_result(<8 x i8> undef)
  ret void
}

declare spir_func <2 x float> @__spirv_SubgroupBitcastShuffleINTEL_float_vector_result(<8 x i8>)

define spir_kernel void @test_float_vector_operand() {
; IR-LABEL: define spir_kernel void @test_float_vector_operand(
; IR: call spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_float_vector_operand(<2 x float> undef)
; ERROR: error: in function 'test_float_vector_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_float_vector_operand(<2 x float> undef)
  ret void
}

declare spir_func <8 x i8> @__spirv_SubgroupBitcastShuffleINTEL_float_vector_operand(<2 x float>)

define spir_kernel void @test_same_components() {
; IR-LABEL: define spir_kernel void @test_same_components(
; IR: call spir_func float @__spirv_SubgroupBitcastShuffleINTEL_same_components(i32 undef)
; ERROR: error: in function 'test_same_components' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func float @__spirv_SubgroupBitcastShuffleINTEL_same_components(i32 undef)
  ret void
}

declare spir_func float @__spirv_SubgroupBitcastShuffleINTEL_same_components(i32)

define spir_kernel void @test_bool_result() {
; IR-LABEL: define spir_kernel void @test_bool_result(
; IR: call spir_func <8 x i1> @__spirv_SubgroupBitcastShuffleINTEL_bool_result(i8 undef)
; ERROR: error: in function 'test_bool_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <8 x i1> @__spirv_SubgroupBitcastShuffleINTEL_bool_result(i8 undef)
  ret void
}

declare spir_func <8 x i1> @__spirv_SubgroupBitcastShuffleINTEL_bool_result(i8)

define spir_kernel void @test_bool_operand() {
; IR-LABEL: define spir_kernel void @test_bool_operand(
; IR: call spir_func i8 @__spirv_SubgroupBitcastShuffleINTEL_bool_operand(<8 x i1> undef)
; ERROR: error: in function 'test_bool_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func i8 @__spirv_SubgroupBitcastShuffleINTEL_bool_operand(<8 x i1> undef)
  ret void
}

declare spir_func i8 @__spirv_SubgroupBitcastShuffleINTEL_bool_operand(<8 x i1>)

define spir_kernel void @test_subbyte_result() {
; IR-LABEL: define spir_kernel void @test_subbyte_result(
; IR: call spir_func <4 x i4> @__spirv_SubgroupBitcastShuffleINTEL_subbyte_result(<2 x i8> undef)
; ERROR: error: in function 'test_subbyte_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <4 x i4> @__spirv_SubgroupBitcastShuffleINTEL_subbyte_result(<2 x i8> undef)
  ret void
}

declare spir_func <4 x i4> @__spirv_SubgroupBitcastShuffleINTEL_subbyte_result(<2 x i8>)

define spir_kernel void @test_wide_result() {
; IR-LABEL: define spir_kernel void @test_wide_result(
; IR: call spir_func i128 @__spirv_SubgroupBitcastShuffleINTEL_wide_result(<4 x i32> undef)
; ERROR: error: in function 'test_wide_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func i128 @__spirv_SubgroupBitcastShuffleINTEL_wide_result(<4 x i32> undef)
  ret void
}

declare spir_func i128 @__spirv_SubgroupBitcastShuffleINTEL_wide_result(<4 x i32>)

define spir_kernel void @test_wide_operand() {
; IR-LABEL: define spir_kernel void @test_wide_operand(
; IR: call spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_wide_operand(i128 undef)
; ERROR: error: in function 'test_wide_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_wide_operand(i128 undef)
  ret void
}

declare spir_func <4 x i32> @__spirv_SubgroupBitcastShuffleINTEL_wide_operand(i128)

define spir_kernel void @test_single_element_result() {
; IR-LABEL: define spir_kernel void @test_single_element_result(
; IR: call spir_func <1 x i32> @__spirv_SubgroupBitcastShuffleINTEL_single_element_result(<2 x i16> undef)
; ERROR: error: in function 'test_single_element_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <1 x i32> @__spirv_SubgroupBitcastShuffleINTEL_single_element_result(<2 x i16> undef)
  ret void
}

declare spir_func <1 x i32> @__spirv_SubgroupBitcastShuffleINTEL_single_element_result(<2 x i16>)

define spir_kernel void @test_single_element_operand() {
; IR-LABEL: define spir_kernel void @test_single_element_operand(
; IR: call spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_single_element_operand(<1 x i32> undef)
; ERROR: error: in function 'test_single_element_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_single_element_operand(<1 x i32> undef)
  ret void
}

declare spir_func <2 x i16> @__spirv_SubgroupBitcastShuffleINTEL_single_element_operand(<1 x i32>)

define spir_kernel void @test_three_element_result() {
; IR-LABEL: define spir_kernel void @test_three_element_result(
; IR: call spir_func <3 x i32> @__spirv_SubgroupBitcastShuffleINTEL_three_element_result(<6 x i16> undef)
; ERROR: error: in function 'test_three_element_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <3 x i32> @__spirv_SubgroupBitcastShuffleINTEL_three_element_result(<6 x i16> undef)
  ret void
}

declare spir_func <3 x i32> @__spirv_SubgroupBitcastShuffleINTEL_three_element_result(<6 x i16>)

define spir_kernel void @test_three_element_operand() {
; IR-LABEL: define spir_kernel void @test_three_element_operand(
; IR: call spir_func <6 x i16> @__spirv_SubgroupBitcastShuffleINTEL_three_element_operand(<3 x i32> undef)
; ERROR: error: in function 'test_three_element_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <6 x i16> @__spirv_SubgroupBitcastShuffleINTEL_three_element_operand(<3 x i32> undef)
  ret void
}

declare spir_func <6 x i16> @__spirv_SubgroupBitcastShuffleINTEL_three_element_operand(<3 x i32>)

define spir_kernel void @test_large_vector_result() {
; IR-LABEL: define spir_kernel void @test_large_vector_result(
; IR: call spir_func <32 x i8> @__spirv_SubgroupBitcastShuffleINTEL_large_vector_result(<16 x i16> undef)
; ERROR: error: in function 'test_large_vector_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <32 x i8> @__spirv_SubgroupBitcastShuffleINTEL_large_vector_result(<16 x i16> undef)
  ret void
}

declare spir_func <32 x i8> @__spirv_SubgroupBitcastShuffleINTEL_large_vector_result(<16 x i16>)

define spir_kernel void @test_large_vector_operand() {
; IR-LABEL: define spir_kernel void @test_large_vector_operand(
; IR: call spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_large_vector_operand(<32 x i8> undef)
; ERROR: error: in function 'test_large_vector_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_large_vector_operand(<32 x i8> undef)
  ret void
}

declare spir_func <16 x i16> @__spirv_SubgroupBitcastShuffleINTEL_large_vector_operand(<32 x i8>)

define spir_kernel void @test_scalable_result() {
; IR-LABEL: define spir_kernel void @test_scalable_result(
; IR: call spir_func <vscale x 4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_scalable_result(i32 undef)
; ERROR: error: in function 'test_scalable_result' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func <vscale x 4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_scalable_result(i32 undef)
  ret void
}

declare spir_func <vscale x 4 x i8> @__spirv_SubgroupBitcastShuffleINTEL_scalable_result(i32)

define spir_kernel void @test_scalable_operand() {
; IR-LABEL: define spir_kernel void @test_scalable_operand(
; IR: call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_scalable_operand(<vscale x 4 x i8> undef)
; ERROR: error: in function 'test_scalable_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_scalable_operand(<vscale x 4 x i8> undef)
  ret void
}

declare spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_scalable_operand(<vscale x 4 x i8>)

define spir_kernel void @test_array_operand() {
; IR-LABEL: define spir_kernel void @test_array_operand(
; IR: call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_array_operand([2 x i16] undef)
; ERROR: error: in function 'test_array_operand' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit integers or vectors of 2, 4, 8, or 16 such integers
  %result = call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_array_operand([2 x i16] undef)
  ret void
}

declare spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_array_operand([2 x i16])

define spir_kernel void @test_same_type() {
; IR-LABEL: define spir_kernel void @test_same_type(
; IR: call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_same_type(i32 undef)
; ERROR: error: in function 'test_same_type' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must differ
  %result = call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_same_type(i32 undef)
  ret void
}

declare spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_same_type(i32)

define spir_kernel void @test_different_size() {
; IR-LABEL: define spir_kernel void @test_different_size(
; IR: call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_different_size(i16 undef)
; ERROR: error: in function 'test_different_size' called indirectly: __spirv_SubgroupBitcastShuffleINTEL: result and operand types must have the same size
  %result = call spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_different_size(i16 undef)
  ret void
}

declare spir_func i32 @__spirv_SubgroupBitcastShuffleINTEL_different_size(i16)

