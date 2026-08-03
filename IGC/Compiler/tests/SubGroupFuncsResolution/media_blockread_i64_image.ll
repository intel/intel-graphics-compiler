;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-sub-group-func-resolution %s -S -o - | FileCheck %s

; The GenISA.simdMediaBlockRead image operand is an "image id" (any-int). Its
; overloaded type must come from the resolved image index (an i32 constant from
; getImageIndex), not from the raw call operand.

define spir_kernel void @test_bread(ptr addrspace(1) %U) {
entry:
; CHECK-LABEL: @test_bread(
; CHECK: call <8 x i16> @llvm.genx.GenISA.simdMediaBlockRead.v8i16.i32(i32 0, i32 %xOffset, i32 %yOffset, i32 1)
;
; CHECK: declare <8 x i16> @llvm.genx.GenISA.simdMediaBlockRead.v8i16.i32(i32, i32, i32, i32)
;
  %0 = addrspacecast ptr addrspace(1) %U to ptr addrspace(4)
  %1 = ptrtoint ptr addrspace(4) %0 to i64
  %call.i = call spir_func <8 x i16> @__builtin_IB_simd_media_block_read_8_h(i64 %1, <2 x i32> zeroinitializer)
  ret void
}

declare spir_func <8 x i16> @__builtin_IB_simd_media_block_read_8_h(i64, <2 x i32>)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{ptr @test_bread, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !12}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test_bread}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 1}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"indexType", i32 0}
!12 = !{!"UseBindlessImage", i1 false}
