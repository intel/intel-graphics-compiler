;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s --implicit-check-not="error:"
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - OpConstantNull of a cooperative matrix type
; (zeroinitializer) resolves to a zero matrix slice.
; ------------------------------------------------

define spir_kernel void @mad_null_operands(ptr addrspace(1) %dst, i64 %stride) {
; CHECK-LABEL: @mad_null_operands(
; CHECK-NEXT:    [[STORE_SLICE:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[A:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[B:%.*]] = alloca <8 x i32>
; CHECK-NEXT:    [[C:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[D:%.*]] = alloca <16 x float>
; CHECK-NEXT:    store <16 x i16> zeroinitializer, ptr [[A]]
; CHECK-NEXT:    store <8 x i32> zeroinitializer, ptr [[B]]
; CHECK-NEXT:    store <16 x float> zeroinitializer, ptr [[C]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_fp16_fp16_fp32_fp32(ptr [[A]], ptr [[B]], ptr [[C]], ptr [[D]])
; CHECK-NEXT:    [[RES:%.*]] = load <16 x float>, ptr [[D]]
; CHECK-NEXT:    store <16 x float> [[RES]], ptr [[STORE_SLICE]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[STORE_SLICE]], i64 %stride, i32 0)
; CHECK-NEXT:    ret void
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHR(target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0) zeroinitializer, target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 1) zeroinitializer, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) zeroinitializer)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHR(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %1, i32 0, i64 %stride, i32 3)
  ret void
}

define spir_kernel void @store_null(ptr addrspace(1) %dst, i64 %stride) {
; CHECK-LABEL: @store_null(
; CHECK-NEXT:    [[SLICE:%.*]] = alloca <16 x float>
; CHECK-NEXT:    store <16 x float> zeroinitializer, ptr [[SLICE]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[SLICE]], i64 %stride, i32 0)
; CHECK-NEXT:    ret void
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHR(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) zeroinitializer, i32 0, i64 %stride, i32 3)
  ret void
}

define spir_kernel void @accumulate_from_null(ptr addrspace(1) %dst, i64 %stride, i32 %n) {
; CHECK-LABEL: @accumulate_from_null(
; CHECK:       loop:
; CHECK-NEXT:    [[ACC:%.*]] = phi <16 x float> [ zeroinitializer, %entry ], [ [[MAD:%.*]], %loop ]
; CHECK:         store <16 x float> [[ACC]], ptr [[C:%[0-9]+]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_fp16_fp16_fp32_fp32(ptr {{%.*}}, ptr {{%.*}}, ptr [[C]], ptr [[D:%[0-9]+]])
; CHECK-NEXT:    [[MAD]] = load <16 x float>, ptr [[D]]
entry:
  br label %loop

loop:
  %acc = phi target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) [ zeroinitializer, %entry ], [ %mad, %loop ]
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %mad = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHR(target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0) zeroinitializer, target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 1) zeroinitializer, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %acc)
  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %loop, label %exit

exit:
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHR(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %mad, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHR(target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0), target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 1), target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2))
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHR(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2), i32, i64, i32)

!igc.functions = !{!0, !3, !4}
!0 = !{ptr @mad_null_operands, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @store_null, !1}
!4 = !{ptr @accumulate_from_null, !1}
