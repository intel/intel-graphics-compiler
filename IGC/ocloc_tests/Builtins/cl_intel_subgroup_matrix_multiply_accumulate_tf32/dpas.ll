;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func float @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8fDv8_ff(float, <8 x float>, float)
declare spir_func <2 x float> @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8fDv8_fDv2_f(float, <8 x float>, <2 x float>)
declare spir_func <4 x float> @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8Dv2_fDv8_fDv4_f(<2 x float>, <8 x float>, <4 x float>)
declare spir_func <8 x float> @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8Dv4_fDv8_fS0_(<4 x float>, <8 x float>, <8 x float>)

define spir_kernel void @test_v1(float %a, <8 x float> %b, float %acc, float addrspace(1)* %c) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v1(
; CHECK:         call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.f32.v8i32(float %acc, float %a, <8 x i32> %{{.+}}, i32 10, i32 10, i32 8, i32 1, i1 false)
  %call = call spir_func float @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8fDv8_ff(float %a, <8 x float> %b, float %acc)
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %c, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

define spir_kernel void @test_v2(float %a, <8 x float> %b, <2 x float> %acc, <2 x float> addrspace(1)* %c) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v2(
; CHECK:         call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.f32.v8i32(<2 x float> %acc, float %a, <8 x i32> %{{.+}}, i32 10, i32 10, i32 8, i32 2, i1 false)
  %call = call spir_func <2 x float> @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8fDv8_fDv2_f(float %a, <8 x float> %b, <2 x float> %acc)
  %arrayidx = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %c, i64 0
  store <2 x float> %call, <2 x float> addrspace(1)* %arrayidx, align 4
  ret void
}

define spir_kernel void @test_v4(<2 x float> %a, <8 x float> %b, <4 x float> %acc, <4 x float> addrspace(1)* %c) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v4(
; CHECK:         call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v2f32.v8i32(<4 x float> %acc, <2 x float> %a, <8 x i32> %{{.+}}, i32 10, i32 10, i32 8, i32 4, i1 false)
  %call = call spir_func <4 x float> @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8Dv2_fDv8_fDv4_f(<2 x float> %a, <8 x float> %b, <4 x float> %acc)
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %c, i64 0
  store <4 x float> %call, <4 x float> addrspace(1)* %arrayidx, align 4
  ret void
}

define spir_kernel void @test_v8(<4 x float> %a, <8 x float> %b, <8 x float> %acc, <8 x float> addrspace(1)* %c) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v8(
; CHECK:         call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v4f32.v8i32(<8 x float> %acc, <4 x float> %a, <8 x i32> %{{.+}}, i32 10, i32 10, i32 8, i32 8, i1 false)
  %call = call spir_func <8 x float> @_Z39intel_sub_group_tf32_tf32_matrix_mad_k8Dv4_fDv8_fS0_(<4 x float> %a, <8 x float> %b, <8 x float> %acc)
  %arrayidx = getelementptr inbounds <8 x float>, <8 x float> addrspace(1)* %c, i64 0
  store <8 x float> %call, <8 x float> addrspace(1)* %arrayidx, align 4
  ret void
}

!100 = !{i32 16}
