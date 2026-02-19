;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --UniformAssumptions -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; UniformAssumptions
; ------------------------------------------------

; Test checks that correctly assumed uniform resource usages are made uniform(by adding readFirstLane)

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_uniform(float %a, float %b, i32* %ptr) {
; CHECK-LABEL: @test_uniform(
; CHECK:    [[TMP1:%.*]] = addrspacecast i32* [[PTR:%.*]] to float addrspace(1245184)*
; CHECK:    [[TMP2:%.*]] = addrspacecast i32* [[PTR]] to i32 addrspace(1638400)*
; CHECK:    [[TMP3:%.*]] = call i1 @llvm.genx.GenISA.is.uniform.p1245184f32(float addrspace(1245184)* [[TMP1]])
; CHECK:    [[TMP4:%.*]] = call i1 @llvm.genx.GenISA.is.uniform.p1638400i32(i32 addrspace(1638400)* [[TMP2]])
; CHECK:    [[TMP5:%.*]] = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK:    [[TMP6:%.*]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[TMP5]])
; CHECK:    [[TMP7:%.*]] = call float addrspace(1245184)* @llvm.genx.GenISA.WaveShuffleIndex.p1245184f32(float addrspace(1245184)* [[TMP1]], i32 [[TMP6]], i32 0)
; CHECK:    [[TMP8:%.*]] = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK:    [[TMP9:%.*]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[TMP8]])
; CHECK:    [[TMP10:%.*]] = call i32 addrspace(1638400)* @llvm.genx.GenISA.WaveShuffleIndex.p1638400i32(i32 addrspace(1638400)* [[TMP2]], i32 [[TMP9]], i32 0)
; CHECK:    [[TMP11:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p1245184f32.p1638400i32(float 0.000000e+00, float [[A:%.*]], float [[B:%.*]], float 0.000000e+00, float 0.000000e+00, float addrspace(1245184)* [[TMP7]], i32 addrspace(1638400)* [[TMP10]], i32 0, i32 0, i32 0)
; CHECK:    ret void
;
  %1 = addrspacecast i32* %ptr to float addrspace(1245184)*
  %2 = addrspacecast i32* %ptr to i32 addrspace(1638400)*
  %3 = call i1 @llvm.genx.GenISA.is.uniform.p1245184f32(float addrspace(1245184)* %1)
  %4 = call i1 @llvm.genx.GenISA.is.uniform.p1638400i32(i32 addrspace(1638400)* %2)
  %5 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p1245184f32.p1638400i32(float 0.000000e+00, float %a, float %b, float 0.000000e+00, float 0.000000e+00, float addrspace(1245184)* %1, i32 addrspace(1638400)* %2, i32 0, i32 0, i32 0)
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p1245184f32.p1638400i32(float, float, float, float, float, float addrspace(1245184)*, i32 addrspace(1638400)*, i32, i32, i32)
declare i1 @llvm.genx.GenISA.is.uniform.p1245184f32(float addrspace(1245184)*)
declare i1 @llvm.genx.GenISA.is.uniform.p1638400i32(i32 addrspace(1638400)*)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1, !2, !3, !4, !5, !6}
!1 = !{!"EnableTextureIndirection", i1 true}
!2 = !{!"EnableSamplerIndirection", i1 true}
!3 = !{!"samplerStateStride", i32 0}
!4 = !{!"samplerStateOffset", i32 0}
!5 = !{!"textureStateStride", i32 0}
!6 = !{!"textureStateOffset", i32 0}
