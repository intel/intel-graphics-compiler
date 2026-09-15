;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformCri 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
;
; An element wise operation on a 4 bit matrix, as element_wise_all_ops.cpp
; performs it. A 4 bit element has no type to name it with, so an access chain
; into a 4 bit matrix addresses the whole byte the elements are packed in: one
; access carries the two elements of a byte, and the slice length is counted in
; those bytes.

; CHECK-LABEL: @element_wise_float4(
; CHECK:         [[SLICE:%.*]] = extractelement <8 x i32> {{.*}}, i64 0
; Access 3 is byte 3 of a dword, so shift it down by 24 bits and truncate. No
; masking of a single element: both elements of the byte belong to the access.
; CHECK:         [[SHIFTED:%.*]] = ashr i32 [[SLICE]], 24
; CHECK:         [[BYTE:%.*]] = trunc i32 [[SHIFTED]] to i8
; CHECK:         [[SUM:%.*]] = add i8 [[BYTE]], 1
; The insert clears the whole byte and merges the updated one back in.
; CHECK:         [[WIDENED:%.*]] = zext i8 [[SUM]] to i32
; CHECK:         [[CLEARED:%.*]] = and i32 {{.*}}, 16777215
; CHECK:         [[SHIFTEDBACK:%.*]] = shl i32 [[WIDENED]], 24
; CHECK:         [[MERGED:%.*]] = or i32 [[CLEARED]], [[SHIFTEDBACK]]
; CHECK-NOT: error:

define spir_kernel void @element_wise_float4() {
entry:
  %sub_b = alloca target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1), align 8
  %sub_b.ascast = addrspacecast ptr %sub_b to ptr addrspace(4)
  %matrix = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z27__spirv_CompositeConstructc(i8 2)
  store target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) %matrix, ptr %sub_b, align 8
  %chain = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1l(ptr addrspace(4) %sub_b.ascast, i64 3)
  %element = load i8, ptr addrspace(4) %chain, align 1
  %updated = add i8 %element, 1
  store i8 %updated, ptr addrspace(4) %chain, align 1
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z27__spirv_CompositeConstructc(i8)

declare spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1l(ptr addrspace(4), i64)

!igc.functions = !{!0}
!0 = !{ptr @element_wise_float4, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
