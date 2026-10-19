;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; SPV_INTEL_tensor_float32_conversion end to end for scalar and every vector width.
; The extension allows "a scalar or vector of 32-bit floating-point type" for
; OpRoundFToTF32INTEL; vector forms used to assert in GenISA_ftotf32 declaration.
; In the final ISA each element of the result is one tf32<-f mov.

; REQUIRES: llvm-spirv, regkeys, cri-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_tensor_float32_conversion -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'DumpASMToConsole=1'" 2>&1 | FileCheck %s

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK-LABEL: //.kernel test_tf32_v1{{$}}
; CHECK-COUNT-1: mov ({{[0-9]+}}|M0) {{r[0-9]+}}.0<1>:tf32 {{r[0-9]+}}.0<1;1,0>:f
; CHECK-NOT: :tf32
define spir_kernel void @test_tf32_v1(float addrspace(1)* %in, float addrspace(1)* %out) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %src.ptr = getelementptr inbounds float, float addrspace(1)* %in, i64 %gid
  %src = load float, float addrspace(1)* %src.ptr
  %res = call spir_func float @_Z25__spirv_RoundFToTF32INTELf(float %src)
  %dst.ptr = getelementptr inbounds float, float addrspace(1)* %out, i64 %gid
  store float %res, float addrspace(1)* %dst.ptr
  ret void
}
declare spir_func float @_Z25__spirv_RoundFToTF32INTELf(float)

; CHECK-LABEL: //.kernel test_tf32_v2{{$}}
; CHECK-COUNT-2: mov ({{[0-9]+}}|M0) {{r[0-9]+}}.0<1>:tf32 {{r[0-9]+}}.0<1;1,0>:f
; CHECK-NOT: :tf32
define spir_kernel void @test_tf32_v2(<2 x float> addrspace(1)* %in, <2 x float> addrspace(1)* %out) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %src.ptr = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %in, i64 %gid
  %src = load <2 x float>, <2 x float> addrspace(1)* %src.ptr
  %res = call spir_func <2 x float> @_Z25__spirv_RoundFToTF32INTELDv2_f(<2 x float> %src)
  %dst.ptr = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %out, i64 %gid
  store <2 x float> %res, <2 x float> addrspace(1)* %dst.ptr
  ret void
}
declare spir_func <2 x float> @_Z25__spirv_RoundFToTF32INTELDv2_f(<2 x float>)

; CHECK-LABEL: //.kernel test_tf32_v3{{$}}
; CHECK-COUNT-3: mov ({{[0-9]+}}|M0) {{r[0-9]+}}.0<1>:tf32 {{r[0-9]+}}.0<1;1,0>:f
; CHECK-NOT: :tf32
define spir_kernel void @test_tf32_v3(<3 x float> addrspace(1)* %in, <3 x float> addrspace(1)* %out) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %src.ptr = getelementptr inbounds <3 x float>, <3 x float> addrspace(1)* %in, i64 %gid
  %src = load <3 x float>, <3 x float> addrspace(1)* %src.ptr
  %res = call spir_func <3 x float> @_Z25__spirv_RoundFToTF32INTELDv3_f(<3 x float> %src)
  %dst.ptr = getelementptr inbounds <3 x float>, <3 x float> addrspace(1)* %out, i64 %gid
  store <3 x float> %res, <3 x float> addrspace(1)* %dst.ptr
  ret void
}
declare spir_func <3 x float> @_Z25__spirv_RoundFToTF32INTELDv3_f(<3 x float>)

; CHECK-LABEL: //.kernel test_tf32_v4{{$}}
; CHECK-COUNT-4: mov ({{[0-9]+}}|M0) {{r[0-9]+}}.0<1>:tf32 {{r[0-9]+}}.0<1;1,0>:f
; CHECK-NOT: :tf32
define spir_kernel void @test_tf32_v4(<4 x float> addrspace(1)* %in, <4 x float> addrspace(1)* %out) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %src.ptr = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %in, i64 %gid
  %src = load <4 x float>, <4 x float> addrspace(1)* %src.ptr
  %res = call spir_func <4 x float> @_Z25__spirv_RoundFToTF32INTELDv4_f(<4 x float> %src)
  %dst.ptr = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %out, i64 %gid
  store <4 x float> %res, <4 x float> addrspace(1)* %dst.ptr
  ret void
}
declare spir_func <4 x float> @_Z25__spirv_RoundFToTF32INTELDv4_f(<4 x float>)

; CHECK-LABEL: //.kernel test_tf32_v8{{$}}
; CHECK-COUNT-8: mov ({{[0-9]+}}|M0) {{r[0-9]+}}.0<1>:tf32 {{r[0-9]+}}.0<1;1,0>:f
; CHECK-NOT: :tf32
define spir_kernel void @test_tf32_v8(<8 x float> addrspace(1)* %in, <8 x float> addrspace(1)* %out) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %src.ptr = getelementptr inbounds <8 x float>, <8 x float> addrspace(1)* %in, i64 %gid
  %src = load <8 x float>, <8 x float> addrspace(1)* %src.ptr
  %res = call spir_func <8 x float> @_Z25__spirv_RoundFToTF32INTELDv8_f(<8 x float> %src)
  %dst.ptr = getelementptr inbounds <8 x float>, <8 x float> addrspace(1)* %out, i64 %gid
  store <8 x float> %res, <8 x float> addrspace(1)* %dst.ptr
  ret void
}
declare spir_func <8 x float> @_Z25__spirv_RoundFToTF32INTELDv8_f(<8 x float>)

; CHECK-LABEL: //.kernel test_tf32_v16{{$}}
; CHECK-COUNT-16: mov ({{[0-9]+}}|M0) {{r[0-9]+}}.0<1>:tf32 {{r[0-9]+}}.0<1;1,0>:f
; CHECK-NOT: :tf32
define spir_kernel void @test_tf32_v16(<16 x float> addrspace(1)* %in, <16 x float> addrspace(1)* %out) {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %src.ptr = getelementptr inbounds <16 x float>, <16 x float> addrspace(1)* %in, i64 %gid
  %src = load <16 x float>, <16 x float> addrspace(1)* %src.ptr
  %res = call spir_func <16 x float> @_Z25__spirv_RoundFToTF32INTELDv16_f(<16 x float> %src)
  %dst.ptr = getelementptr inbounds <16 x float>, <16 x float> addrspace(1)* %out, i64 %gid
  store <16 x float> %res, <16 x float> addrspace(1)* %dst.ptr
  ret void
}
declare spir_func <16 x float> @_Z25__spirv_RoundFToTF32INTELDv16_f(<16 x float>)
