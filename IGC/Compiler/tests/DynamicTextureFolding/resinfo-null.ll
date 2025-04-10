;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -enable-debugify -regkey DisableDynamicResInfoFolding=0 -igc-dynamic-texture-folding -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DynamicTextureFolding
; ------------------------------------------------

; Test checks resinfo folding for GFXSURFACESTATE_SURFACETYPE_NULL

; Debug info related checks
; CHECK-NOT: WARNING: Missing line {1|3|5|7}
; CHECK: CheckModuleDebugify: PASS


define void @test(i32 %src1, ptr %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p131077(ptr addrspace(131077) null, i32 [[SRC1:%.*]])
; CHECK:    store i32 0, ptr [[DST:%[A-z0-9]*]]
; CHECK:    store i32 0, ptr [[DST]]
; CHECK:    store i32 0, ptr [[DST]]
; CHECK:    store i32 0, ptr [[DST]]
; CHECK:    ret void
;
  %1 = call <4 x i32> @llvm.genx.GenISA.resinfoptr.p131077(ptr addrspace(131077) null, i32 %src1)
  %2 = extractelement <4 x i32> %1, i32 0
  store i32 %2, ptr %dst, align 4
  %3 = extractelement <4 x i32> %1, i32 1
  store i32 %3, ptr %dst, align 4
  %4 = extractelement <4 x i32> %1, i32 2
  store i32 %4, ptr %dst, align 4
  %5 = extractelement <4 x i32> %1, i32 3
  store i32 %5, ptr %dst, align 4
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.resinfoptr.p131077(ptr addrspace(131077), i32)

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineResInfoData", !2}
!2 = !{!"inlineResInfoDataVec[0]", !4, !5, !6, !7, !8, !9, !10, !11, !12}
!4 = !{!"textureID", i32 5}
!5 = !{!"SurfaceType", i32 7}
!6 = !{!"WidthOrBufferSize", i32 1}
!7 = !{!"Height", i32 2}
!8 = !{!"Depth", i32 3}
!9 = !{!"SurfaceArray", i32 4}
!10 = !{!"QWidth", i32 5}
!11 = !{!"QHeight", i32 6}
!12 = !{!"MipCount", i32 7}
