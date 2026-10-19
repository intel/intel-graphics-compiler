;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers --enable-debugify -platformpvc --igc-arith-funcs-translation -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DpasFuncsResolution
; ------------------------------------------------

; Lower tf32 and packed bf16 convert builtins, scalar and vector forms.
; The vector forms are reached e.g. from __spirv_RoundFToTF32INTEL(float4) and
; intel_convert_f32_to_bf16_packed(float4, float4).

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_tf32(float %s, <2 x float> %v2, <3 x float> %v3, <16 x float> %v16, ptr %dst) {
; CHECK-LABEL: @test_tf32(
; CHECK:    [[TF32_1:%.*]] = call float @llvm.genx.GenISA.ftotf32.f32.f32(float [[S:%.*]], i32 0)
; CHECK:    [[TF32_2:%.*]] = call <2 x float> @llvm.genx.GenISA.ftotf32.v2f32.v2f32(<2 x float> [[V2:%.*]], i32 0)
; CHECK:    [[TF32_3:%.*]] = call <3 x float> @llvm.genx.GenISA.ftotf32.v3f32.v3f32(<3 x float> [[V3:%.*]], i32 0)
; CHECK:    [[TF32_16:%.*]] = call <16 x float> @llvm.genx.GenISA.ftotf32.v16f32.v16f32(<16 x float> [[V16:%.*]], i32 0)
; CHECK-NOT: __builtin_IB_ftotf32
; CHECK:    ret void
;
  %1 = call float @__builtin_IB_ftotf32_1(float %s)
  store float %1, ptr %dst, align 4
  %2 = call <2 x float> @__builtin_IB_ftotf32_2(<2 x float> %v2)
  store <2 x float> %2, ptr %dst, align 8
  %3 = call <3 x float> @__builtin_IB_ftotf32_3(<3 x float> %v3)
  store <3 x float> %3, ptr %dst, align 16
  %4 = call <16 x float> @__builtin_IB_ftotf32_16(<16 x float> %v16)
  store <16 x float> %4, ptr %dst, align 64
  ret void
}

define spir_kernel void @test_2fto2bf(float %a, float %b, <4 x float> %va, <4 x float> %vb, <16 x float> %wa, <16 x float> %wb, ptr %dst) {
; CHECK-LABEL: @test_2fto2bf(
; CHECK:    [[BF_1:%.*]] = call i32 @llvm.genx.GenISA.2fto2bf.i32.f32(float [[A:%.*]], float [[B:%.*]], i32 0)
; CHECK:    [[BF_4:%.*]] = call <4 x i32> @llvm.genx.GenISA.2fto2bf.v4i32.v4f32(<4 x float> [[VA:%.*]], <4 x float> [[VB:%.*]], i32 0)
; CHECK:    [[BF_4_RTZ:%.*]] = call <4 x i32> @llvm.genx.GenISA.2fto2bf.v4i32.v4f32(<4 x float> [[VA]], <4 x float> [[VB]], i32 3)
; CHECK:    [[BF_16:%.*]] = call <16 x i32> @llvm.genx.GenISA.2fto2bf.v16i32.v16f32(<16 x float> [[WA:%.*]], <16 x float> [[WB:%.*]], i32 0)
; CHECK-NOT: __builtin_IB_2fto2bf
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_2fto2bf_1(float %a, float %b)
  store i32 %1, ptr %dst, align 4
  %2 = call <4 x i32> @__builtin_IB_2fto2bf_4(<4 x float> %va, <4 x float> %vb)
  store <4 x i32> %2, ptr %dst, align 16
  %3 = call <4 x i32> @__builtin_IB_2fto2bf_rtz_4(<4 x float> %va, <4 x float> %vb)
  store <4 x i32> %3, ptr %dst, align 16
  %4 = call <16 x i32> @__builtin_IB_2fto2bf_16(<16 x float> %wa, <16 x float> %wb)
  store <16 x i32> %4, ptr %dst, align 64
  ret void
}

declare float @__builtin_IB_ftotf32_1(float)
declare <2 x float> @__builtin_IB_ftotf32_2(<2 x float>)
declare <3 x float> @__builtin_IB_ftotf32_3(<3 x float>)
declare <16 x float> @__builtin_IB_ftotf32_16(<16 x float>)
declare i32 @__builtin_IB_2fto2bf_1(float, float)
declare <4 x i32> @__builtin_IB_2fto2bf_4(<4 x float>, <4 x float>)
declare <4 x i32> @__builtin_IB_2fto2bf_rtz_4(<4 x float>, <4 x float>)
declare <16 x i32> @__builtin_IB_2fto2bf_16(<16 x float>, <16 x float>)
