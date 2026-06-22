;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-sub-group-func-resolution %s -S -o - | FileCheck %s

; A media block read whose result is a vector (e.g. int4) must lower to
; GenISA.MediaBlockRead overloaded on the vector return type. The intrinsic is
; defined with an "any integer" return type, which - matching LLVM's iAny
; semantics - accepts both scalar integers and integer vectors. Regression test
; for an over-strict type verifier that asserted on the vector return type.

define spir_kernel void @test_kernel_media_block_read_vec(ptr addrspace(1) %image) {
entry:
; CHECK: [[IMG:%.*]] = ptrtoint ptr addrspace(1) %image to i64
; CHECK-NEXT: [[IMG_TRUNC:%.*]] = trunc i64 [[IMG]] to i32
; CHECK: call <4 x i32> @llvm.genx.GenISA.MediaBlockRead.v4i32.i32(i32 [[IMG_TRUNC]], i32 %xOffset, i32 %yOffset, i32 0, i32 4, i32 4)
;
; CHECK: declare <4 x i32> @llvm.genx.GenISA.MediaBlockRead.v4i32.i32(i32, i32, i32, i32, i32, i32)
;
  %0 = ptrtoint ptr addrspace(1) %image to i64
  %1 = trunc i64 %0 to i32
  %call.i = call spir_func <4 x i32> @__builtin_IB_media_block_read(i32 %1, <2 x i32> zeroinitializer, i32 4, i32 4)
  ret void
}

declare spir_func <4 x i32> @__builtin_IB_media_block_read(i32, <2 x i32>, i32, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{ptr @test_kernel_media_block_read_vec, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !10}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test_kernel_media_block_read_vec}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8}
!8 = !{!"argAllocMDListVec[0]", !9}
!9 = !{!"type", i32 0}
!10 = !{!"UseBindlessImage", i1 true}
