;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

; CHECK-LABEL: @mad_builtin_half_float(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <32 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <8 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x float>
; CHECK-NEXT:    store <32 x i16>{{.*}} ptr [[TMP2]]
; CHECK-NEXT:    store <8 x i32>{{.*}} ptr [[TMP3]]
; CHECK-NEXT:    store <16 x float> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_i32_fp16_fp32_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x float>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x float> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_half_float(ptr %src, i64 %stride, ptr %dst) {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z26__spirv_CompositeConstructf(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 0) @_Z76__spirv_CompositeConstructf(float 1.0)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", half,  3, 16, 16, 1) @_Z80__spirv_CompositeConstructh(half -1.0)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv.CooperativeMatrixKHR._float_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR._half_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2(target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 0) %2, target("spirv.CooperativeMatrixKHR", half,  3, 16, 16, 1) %3, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %1)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z26__spirv_CompositeConstructf(float)

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 0) @_Z76__spirv_CompositeConstructf(float)

declare spir_func target("spirv.CooperativeMatrixKHR", half,  3, 16, 16, 1) @_Z80__spirv_CompositeConstructh(half)

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv.CooperativeMatrixKHR._float_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR._half_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2(target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 0), target("spirv.CooperativeMatrixKHR", half,  3, 16, 16, 1), target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2))

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2ili(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @mad_builtin_half_float, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
