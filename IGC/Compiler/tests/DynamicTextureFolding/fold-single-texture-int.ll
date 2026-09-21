;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -debugify -igc-dynamic-texture-folding -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DynamicTextureFolding
; ------------------------------------------------
;
; FoldSingleTextureValue replaces a texture load with the constant values that
; UMD published in the inlineDynTextures metadata. This test pins the INTEGER
; result path: the load returns <4 x i32>, so the pass must take the
; isIntOrIntVectorTy branch and materialise ConstantInt, not ConstantFP.
;
; The texture address space (524293) has isSurfaceRes = 0, so
; DecodeAS4GFXResourceType reports a non-array resource and skipBoundaryCheck
; stays 1 -- each extractelement is replaced by a bare constant with no
; boundary compare and no select.

; Debug info related checks
; CHECK-NOT: WARNING: Missing line
; CHECK: CheckModuleDebugify: PASS

define void @test_fold_int(ptr %dst) {
; CHECK-LABEL: @test_fold_int(
; CHECK:    store i32 11, ptr [[DST:%.*]], align 4
; CHECK:    store i32 22, ptr [[DST]], align 4
; CHECK:    store i32 33, ptr [[DST]], align 4
; CHECK:    store i32 44, ptr [[DST]], align 4
; CHECK:    ret void
;
  %1 = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32(i32 1, i32 2, i32 3, i32 4, ptr addrspace(196609) null, ptr addrspace(524293) inttoptr (i64 5 to ptr addrspace(524293)), i32 5, i32 6, i32 7)
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

declare <4 x i32> @llvm.genx.GenISA.ldptr.v4i32(i32, i32, i32, i32, ptr addrspace(196609), ptr addrspace(524293), i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineDynTextures", !2, !3}
!2 = !{!"inlineDynTexturesMap[0]", i32 5}
!3 = !{!"inlineDynTexturesValue[0]", !4, !5, !6, !7}
!4 = !{!"inlineDynTexturesValue[0]Vec[0]", i32 11}
!5 = !{!"inlineDynTexturesValue[0]Vec[1]", i32 22}
!6 = !{!"inlineDynTexturesValue[0]Vec[2]", i32 33}
!7 = !{!"inlineDynTexturesValue[0]Vec[3]", i32 44}
