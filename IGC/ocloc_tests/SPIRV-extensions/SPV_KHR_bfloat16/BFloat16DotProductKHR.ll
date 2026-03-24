;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s
; COM: Execute ocloc second time, this time without DumpVISAASMToConsole flag, to ensure that E2E compilation does not crash.
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=16' -cl-intel-library-compilation"

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: .function "OpDot_vec2
define spir_func bfloat @OpDot_vec2(<2 x bfloat> %vec1, <2 x bfloat> %vec2) {
  ; CHECK: mul (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,16)<1;1,0> [[SRC]](1,16)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST]](0,0)<1> [[SRC]](1,0)<1;1,0> [[SRC]](0,0)<1;1,0> [[DST]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z11__spirv_DotDv2_DF16bS_(<2 x bfloat> %vec1, <2 x bfloat> %vec2)
  ret bfloat %result
}

; CHECK-LABEL: .function "OpDot_vec3
define spir_func bfloat @OpDot_vec3(<3 x bfloat> %vec1, <3 x bfloat> %vec2) {
  ; CHECK: mul (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](1,0)<1;1,0> [[SRC]](3,0)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST]](0,0)<1> [[SRC]](2,16)<1;1,0> [[SRC]](0,16)<1;1,0> [[DST]](0,0)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST]](0,0)<1> [[SRC]](2,0)<1;1,0> [[SRC]](0,0)<1;1,0> [[DST]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z11__spirv_DotDv3_DF16bS_(<3 x bfloat> %vec1, <3 x bfloat> %vec2)
  ret bfloat %result
}

; CHECK-LABEL: .function "OpDot_vec4
define spir_func bfloat @OpDot_vec4(<4 x bfloat> %vec1, <4 x bfloat> %vec2) {
  ; CHECK: mul (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](1,16)<1;1,0> [[SRC]](3,16)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST]](0,0)<1> [[SRC]](3,0)<1;1,0> [[SRC]](1,0)<1;1,0> [[DST]](0,0)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST]](0,0)<1> [[SRC]](2,16)<1;1,0> [[SRC]](0,16)<1;1,0> [[DST]](0,0)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST]](0,0)<1> [[SRC]](2,0)<1;1,0> [[SRC]](0,0)<1;1,0> [[DST]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z11__spirv_DotDv4_DF16bS_(<4 x bfloat> %vec1, <4 x bfloat> %vec2)
  ret bfloat %result
}

declare spir_func bfloat @_Z11__spirv_DotDv2_DF16bS_(<2 x bfloat>, <2 x bfloat>)
declare spir_func bfloat @_Z11__spirv_DotDv3_DF16bS_(<3 x bfloat>, <3 x bfloat>)
declare spir_func bfloat @_Z11__spirv_DotDv4_DF16bS_(<4 x bfloat>, <4 x bfloat>)
