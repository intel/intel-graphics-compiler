;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -enable-debugify -igc-joint-matrix-resolution -S --platformdg2 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; Testing whether load + store works fine with target extension types on opaque-pointers mode
; ------------------------------------------------


define spir_kernel void @test_jm(ptr %t3_a, ptr %t3_dst) {
  call void @load_store_acc_transpose(ptr %t3_a, ptr %t3_dst)
  ret void
}

%spirv.CooperativeMatrixKHR._float_3_8_8_2 = type { target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) }
define void @load_store_acc_transpose(ptr %a, ptr %dst) {
; CHECK-LABEL: define void @load_store_acc_transpose(
; CHECK: [[PTR1:%.*]] = alloca <8 x float>
; CHECK: [[PTR2:%.*]] = alloca <8 x float>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_ColumnMajor_8x8_i32_8_generic_v8i8_pi32_i32(ptr [[PTR2]], ptr %a, i64 64, i32 0)
; CHECK: [[MATRIX:%.*]] = load <8 x float>, ptr [[PTR2]]
; CHECK: store <8 x float> [[MATRIX]], ptr [[PTR1]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_ColumnMajor_8x8_i32_8_generic_pi64_v8i8(ptr %dst, ptr [[PTR1]], i64 64, i32 0)
; CHECK: ret void
; CHECK-NOT: error
;

%1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__char_2_48_12_3PU3AS1fili(ptr %a, i32 1, i64 64, i32 0) #0
call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4sPU3AS143__spirv_CooperativeMatrixKHR__int_3_8_8_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) %1, i32 1, i64 64, i32 0) #0
ret void
}
; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__char_2_48_12_3PU3AS1fili(ptr, i32, i64, i32) #0
; Function Attrs: nounwind
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4sPU3AS143__spirv_CooperativeMatrixKHR__int_3_8_8_2ili(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2), i32, i64, i32) #0

!igc.functions = !{!0}
!0 = !{ptr @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
