;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-joint-matrix-resolution -S --platformdg2 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_jm(float addrspace(1)* %t3_a, float addrspace(1)* %t3_dst) {
  call void @load_store_acc_transpose(float addrspace(1)* %t3_a, float addrspace(1)* %t3_dst)
  ret void
}

%spirv.CooperativeMatrixKHR._float_3_8_8_2 = type opaque
define void @load_store_acc_transpose(float addrspace(1)* %a, float addrspace(1)* %dst) {
; CHECK-LABEL: define void @load_store_acc_transpose(
; CHECK: [[TMP4:%.*]] = alloca <8 x float>
; CHECK: [[PTR:%.*]] = alloca <8 x float>
; CHECK: [[MATPTR:%.*]] = bitcast <8 x float>* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_ColumnMajor_8x8_i32_8_global_v8i8_pi32_i32(i8* [[MATPTR]], float addrspace(1)* %a, i64 64, i32 0)
; CHECK: [[MATRIX:%.*]] = load <8 x float>, <8 x float>* [[PTR]]
; CHECK: store <8 x float> [[MATRIX]], <8 x float>* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast <8 x float>* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_ColumnMajor_8x8_i32_8_global_pi64_v8i8(float addrspace(1)* %dst, i8* [[TMP5]], i64 64, i32 0)
; CHECK: ret void
; CHECK-NOT: error
;
%1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_8_8_2 addrspace(1)* @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__char_2_48_12_3PU3AS1fili(float addrspace(1)* %a, i32 1, i64 64, i32 0) #0
call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4sPU3AS143__spirv_CooperativeMatrixKHR__int_3_8_8_2ili(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_8_8_2 addrspace(1)* %1, i32 1, i64 64, i32 0) #0
ret void
}
; Function Attrs: nounwind
declare spir_func %spirv.CooperativeMatrixKHR._float_3_8_8_2 addrspace(1)* @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__char_2_48_12_3PU3AS1fili(float addrspace(1)*, i32, i64, i32) #0
; Function Attrs: nounwind
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4sPU3AS143__spirv_CooperativeMatrixKHR__int_3_8_8_2ili(float addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_8_8_2 addrspace(1)*, i32, i64, i32) #0

!igc.functions = !{!0}
!0 = !{void (float addrspace(1)*, float addrspace(1)*)* @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
