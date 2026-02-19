;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -igc-dynamic-texture-folding -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DynamicTextureFolding
; ------------------------------------------------

; Debug info related checks
; CHECK-NOT: WARNING: Missing line {1|3|5|7}
; CHECK: CheckModuleDebugify: PASS


define void @test(float %src1, float %src2, float %src3, float* %dst) {
; CHECK-LABEL: @test(
; CHECK:    store float 0.000000e+00, float* [[DST:%.*]], align 4
; CHECK:    store float 0.000000e+00, float* [[DST]], align 4
; CHECK:    store float 0.000000e+00, float* [[DST]], align 4
; CHECK:    store float 0.000000e+00, float* [[DST]], align 4
; CHECK:    ret void
;
  %1 = call <4 x float> @llvm.genx.GenISA.ldptr.4f32(i32 1, i32 2, i32 3, i32 4, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 5, i32 6, i32 7)
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

declare <4 x float> @llvm.genx.GenISA.ldptr.4f32(i32, i32, i32, i32, i8 addrspace(196609)*, i8 addrspace(524293)*, i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineDynTextures", !2, !3}
!2 = !{!"inlineDynTexturesMap[0]", i32 5}
!3 = !{!"inlineDynTexturesValue[0]", !4, !5, !6, !7}
!4 = !{!"inlineDynTexturesValue[0]Vec[0]", i32 0}
!5 = !{!"inlineDynTexturesValue[0]Vec[1]", i32 0}
!6 = !{!"inlineDynTexturesValue[0]Vec[2]", i32 0}
!7 = !{!"inlineDynTexturesValue[0]Vec[3]", i32 0}
