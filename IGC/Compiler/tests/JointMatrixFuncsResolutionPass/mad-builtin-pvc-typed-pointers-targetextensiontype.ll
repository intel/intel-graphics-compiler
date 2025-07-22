;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

; CHECK-LABEL: @mad_builtin(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} <8 x i16>* [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} <4 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, <16 x i32>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <8 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i32>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i32>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_s8_s8_i32_i32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, <16 x i32>* [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], <16 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(i32 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 0) @_Z79__spirv_CompositeConstructi(i8 1)
  %3 = call spir_func target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 1) @_Z80__spirv_CompositeConstructi(i8 -1)
  %4 = call spir_func target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2) @_Z27__spirv_JointMatrixMadINTELPU3AS141__spirv_JointMatrixINTEL__char_16_16_0_3_0PU3AS142__spirv_JointMatrixINTEL__char_16_16_0_3_1PU3AS140__spirv_JointMatrixINTEL__int_16_16_3_3_2i(target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 0) %2, target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 1) %3, target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2) %1, i32 3)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__int_16_16_3_3_2liii(i32 addrspace(1)* %dst, target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2) %4, i64 %stride, i32 0, i32 3, i32 0)
  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructi(i32)
declare spir_func target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 0) @_Z79__spirv_CompositeConstructi(i8)
declare spir_func target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 1) @_Z80__spirv_CompositeConstructi(i8)
declare spir_func target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2) @_Z27__spirv_JointMatrixMadINTELPU3AS141__spirv_JointMatrixINTEL__char_16_16_0_3_0PU3AS142__spirv_JointMatrixINTEL__char_16_16_0_3_1PU3AS140__spirv_JointMatrixINTEL__int_16_16_3_3_2i(target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 0), target("spirv.JointMatrixINTEL", i8, 16, 16, 0, 3, 1), target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2), i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__int_16_16_3_3_2liii(i32 addrspace(1)*, target("spirv.JointMatrixINTEL", i32, 16, 16, 3, 3, 2), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(1)*, i64, i32 addrspace(1)*)* @mad_builtin, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
