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
; Negative case for FoldSingleTextureValue: the module DOES publish
; inlineDynTextures, so the pass runs, but it publishes them for texture
; index 7 while the load reads texture index 5. The lookup must miss and the
; load must survive untouched.
;
; This guards the lookup itself. Without it, a pass that folded on "any
; inlineDynTextures present" rather than on a matching index would still pass
; every other test in this directory.

; Debug info related checks
; CHECK-NOT: WARNING: Missing line
; CHECK: CheckModuleDebugify: PASS

define void @test_index_miss(ptr %dst) {
; CHECK-LABEL: @test_index_miss(
; CHECK:    [[LD:%.*]] = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32
; CHECK:    [[E0:%.*]] = extractelement <4 x i32> [[LD]], i32 0
; CHECK:    store i32 [[E0]], ptr %dst, align 4
; CHECK:    ret void
;
  %1 = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32(i32 1, i32 2, i32 3, i32 4, ptr addrspace(196609) null, ptr addrspace(524293) inttoptr (i64 5 to ptr addrspace(524293)), i32 5, i32 6, i32 7)
  %2 = extractelement <4 x i32> %1, i32 0
  store i32 %2, ptr %dst, align 4
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.ldptr.v4i32(i32, i32, i32, i32, ptr addrspace(196609), ptr addrspace(524293), i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineDynTextures", !2, !3}
!2 = !{!"inlineDynTexturesMap[0]", i32 7}
!3 = !{!"inlineDynTexturesValue[0]", !4, !5, !6, !7}
!4 = !{!"inlineDynTexturesValue[0]Vec[0]", i32 11}
!5 = !{!"inlineDynTexturesValue[0]Vec[1]", i32 22}
!6 = !{!"inlineDynTexturesValue[0]Vec[2]", i32 33}
!7 = !{!"inlineDynTexturesValue[0]Vec[3]", i32 44}
