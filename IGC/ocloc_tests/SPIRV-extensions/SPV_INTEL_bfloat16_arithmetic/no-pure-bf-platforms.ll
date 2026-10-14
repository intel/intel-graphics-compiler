;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: bmg-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device bmg -options "-igc_opts 'DumpASMToConsole=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s --check-prefix=FMA
; RUN: ocloc compile -spirv_input -file %t.spv -device bmg -options "-igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s --check-prefix=TANH
; RUN: %if pvc-supported %{ ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'DumpASMToConsole=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s --check-prefix=FMA %}
; RUN: %if pvc-supported %{ ocloc compile -spirv_input -file %t.spv -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s --check-prefix=TANH %}

; Platforms without pure BF arithmetic.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; fma must round once, so the product is not rounded to bfloat.
; FMA-LABEL: _test_spirv_ocl_fma:
; FMA-NOT: mul (16|M0)
; FMA: mad (16|M0) [[RES:r[0-9]+\.[0-9]+]]<1>:f
; FMA: mov (16|M0) {{r[0-9]+\.[0-9]+}}<1>:bf [[RES]]<1;1,0>:f
define spir_func bfloat @test_spirv_ocl_fma(bfloat %a, bfloat %b, bfloat %c) {
  %r = call spir_func bfloat @_Z15__spirv_ocl_fmaDF16bDF16bDF16b(bfloat %a, bfloat %b, bfloat %c)
  ret bfloat %r
}

; There is no native tanh, so the float implementation is used.
; TANH-LABEL: .function "test_spirv_ocl_tanh
; TANH-NOT: tanh (M1
; TANH: fret (M1
define spir_func bfloat @test_spirv_ocl_tanh(bfloat %a) {
  %r = call spir_func bfloat @_Z16__spirv_ocl_tanhDF16b(bfloat %a)
  ret bfloat %r
}

declare spir_func bfloat @_Z15__spirv_ocl_fmaDF16bDF16bDF16b(bfloat, bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_tanhDF16b(bfloat)
