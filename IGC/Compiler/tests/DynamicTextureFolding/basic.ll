;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -igc-dynamic-texture-folding -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DynamicTextureFolding
; ------------------------------------------------

; Debug info related checks
; CHECK-NOT: WARNING: Missing line {1|3|5|7}
; CHECK: CheckModuleDebugify: PASS


define void @test(float %src1, float %src2, float %src3, float* %dst) {
; CHECK-LABEL: @test(
; CHECK:    store float 0x36B0000000000000, float* [[DST:%.*]], align 4
; CHECK:    store float 0.000000e+00, float* [[DST]], align 4
; CHECK:    store float 0.000000e+00, float* [[DST]], align 4
; CHECK:    store float 0.000000e+00, float* [[DST]], align 4
; CHECK:    ret void
;
  %1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %src1, float %src2, float %src3, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
  %2 = extractelement <4 x float> %1, i32 0
  store float %2, float* %dst, align 4
  %3 = extractelement <4 x float> %1, i32 1
  store float %3, float* %dst, align 4
  %4 = extractelement <4 x float> %1, i32 2
  store float %4, float* %dst, align 4
  %5 = extractelement <4 x float> %1, i32 3
  store float %5, float* %dst, align 4
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float, float, float, float, float, float, i8 addrspace(196609)*, i8 addrspace(524293)*, i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineDynTextures", !2, !3}
!2 = !{!"inlineDynTexturesMap[0]", i32 1}
!3 = !{!"inlineDynTexturesValue[0]", !4}
!4 = !{i32 0, i32 2, i32 1, i32 1}
