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
; FoldSingleTextureValue on an ARRAY texture, i.e. the boundary-checked path.
;
; The texture address space below is 8945669, which decodes as
; isSurfaceRes = 1 and resDimType = 4 (DIM_2D_ARRAY_TYPE), so the pass clears
; skipBoundaryCheck. It must then emit an ICmp of the array index (operand 3)
; against inlineDynTexturesValue[0]Vec[7] -- the array size published by UMD --
; and guard every folded constant behind a select, yielding 0 when the index
; is out of bounds.
;
; The array index is a function argument on purpose: a constant would let
; IRBuilder fold the compare and the select away, hiding the emitted shape.

; Debug info related checks
; CHECK-NOT: WARNING: Missing line
; CHECK: CheckModuleDebugify: PASS

define void @test_fold_array_bounds(i32 %arrayIdx, ptr %dst) {
; The guards are built with a single insertion point, so the selects appear in
; reverse element order and ahead of the (now dead) texture load.
; CHECK-LABEL: @test_fold_array_bounds(
; CHECK:    [[CMP:%.*]] = icmp slt i32 %arrayIdx, 10
; CHECK:    [[S1:%.*]] = select i1 [[CMP]], i32 22, i32 0
; CHECK:    [[S0:%.*]] = select i1 [[CMP]], i32 11, i32 0
; CHECK:    store i32 [[S0]], ptr %dst, align 4
; CHECK:    store i32 [[S1]], ptr %dst, align 4
; CHECK:    ret void
;
  %1 = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32(i32 1, i32 2, i32 3, i32 %arrayIdx, ptr addrspace(196609) null, ptr addrspace(8945669) inttoptr (i64 5 to ptr addrspace(8945669)), i32 5, i32 6, i32 7)
  %2 = extractelement <4 x i32> %1, i32 0
  store i32 %2, ptr %dst, align 4
  %3 = extractelement <4 x i32> %1, i32 1
  store i32 %3, ptr %dst, align 4
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.ldptr.v4i32(i32, i32, i32, i32, ptr addrspace(196609), ptr addrspace(8945669), i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineDynTextures", !2, !3}
!2 = !{!"inlineDynTexturesMap[0]", i32 5}
!3 = !{!"inlineDynTexturesValue[0]", !4, !5, !6, !7, !8, !9, !10, !11}
!4 = !{!"inlineDynTexturesValue[0]Vec[0]", i32 11}
!5 = !{!"inlineDynTexturesValue[0]Vec[1]", i32 22}
!6 = !{!"inlineDynTexturesValue[0]Vec[2]", i32 33}
!7 = !{!"inlineDynTexturesValue[0]Vec[3]", i32 44}
!8 = !{!"inlineDynTexturesValue[0]Vec[4]", i32 0}
!9 = !{!"inlineDynTexturesValue[0]Vec[5]", i32 0}
!10 = !{!"inlineDynTexturesValue[0]Vec[6]", i32 0}
!11 = !{!"inlineDynTexturesValue[0]Vec[7]", i32 10}
