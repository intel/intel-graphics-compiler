;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-long-vector-builtin-scalarization -S < %s | FileCheck %s

; Which calls the pass rewrites: a call is scalarized only if BiF lacks its
; vector variant and defines the scalar one, decided by BiF's symbol table and
; not by the builtin name or the vector width. Builtins without an elementwise
; vector form either have a dedicated lowering or are rejected at widths OpenCL
; C doesn't define (invalid-spirv.ll).

target triple = "spir64-unknown-unknown"

; BiF defines the vector variant, so the call stays.
; CHECK-LABEL: define spir_func <4 x float> @bif_width(
; CHECK:         call spir_func <4 x float> @_Z15__spirv_ocl_sinDv4_f(<4 x float> %x)
define spir_func <4 x float> @bif_width(<4 x float> %x) {
  %r = call spir_func <4 x float> @_Z15__spirv_ocl_sinDv4_f(<4 x float> %x)
  ret <4 x float> %r
}

; BiF lacks this vector variant even at a standard width, so it is scalarized.
; CHECK-LABEL: define spir_func <4 x i32> @missing_bif_width(
; CHECK-COUNT-4: call spir_func i32 @_Z27__spirv_GroupNonUniformIAddiii(i32 3, i32 0, i32
define spir_func <4 x i32> @missing_bif_width(<4 x i32> %x) {
  %r = call spir_func <4 x i32> @_Z27__spirv_GroupNonUniformIAddiiDv4_i(i32 3, i32 0, <4 x i32> %x)
  ret <4 x i32> %r
}

; The `32` in the name isn't a return type width.
; CHECK-LABEL: define spir_func <32 x float> @digits_in_name(
; CHECK:         call spir_func float @_Z25__spirv_RoundFToTF32INTELf(float
; CHECK-NOT:     call {{.*}}RoundFToTFINTEL
define spir_func <32 x float> @digits_in_name(<32 x float> %x) {
  %r = call spir_func <32 x float> @_Z25__spirv_RoundFToTF32INTELDv32_f(<32 x float> %x)
  ret <32 x float> %r
}

; Not a SPIR-V builtin.
; CHECK-LABEL: define spir_func <5 x float> @not_a_builtin(
; CHECK:         call spir_func <5 x float> @_Z4userDv5_f(<5 x float> %x)
define spir_func <5 x float> @not_a_builtin(<5 x float> %x) {
  %r = call spir_func <5 x float> @_Z4userDv5_f(<5 x float> %x)
  ret <5 x float> %r
}

; BiF has no 3-element shuffle, so the dedicated lowering applies.
; CHECK-LABEL: define spir_func <3 x float> @shuffle_width3(
; CHECK:         urem i32 {{%.*}}, 3
; CHECK-NOT:     call
define spir_func <3 x float> @shuffle_width3(<3 x float> %x, <3 x i32> %m) {
  %r = call spir_func <3 x float> @_Z19__spirv_ocl_shuffleDv3_fDv3_j(<3 x float> %x, <3 x i32> %m)
  ret <3 x float> %r
}

; BiF implements the bfloat variant on i16 and BfloatBuiltinsResolution
; redirects the scalar call to it after this pass.
; CHECK-LABEL: define spir_func <5 x bfloat> @bfloat_via_i16(
; CHECK-COUNT-5: call spir_func bfloat @_Z21__spirv_FSigmoidINTELDF16b(bfloat
define spir_func <5 x bfloat> @bfloat_via_i16(<5 x bfloat> %x) {
  %r = call spir_func <5 x bfloat> @_Z21__spirv_FSigmoidINTELDv5_DF16b(<5 x bfloat> %x)
  ret <5 x bfloat> %r
}

declare spir_func <4 x float> @_Z15__spirv_ocl_sinDv4_f(<4 x float>)
declare spir_func <5 x float> @_Z4userDv5_f(<5 x float>)
declare spir_func <4 x i32> @_Z27__spirv_GroupNonUniformIAddiiDv4_i(i32, i32, <4 x i32>)
declare spir_func <32 x float> @_Z25__spirv_RoundFToTF32INTELDv32_f(<32 x float>)
declare spir_func <3 x float> @_Z19__spirv_ocl_shuffleDv3_fDv3_j(<3 x float>, <3 x i32>)
declare spir_func <5 x bfloat> @_Z21__spirv_FSigmoidINTELDv5_DF16b(<5 x bfloat>)
