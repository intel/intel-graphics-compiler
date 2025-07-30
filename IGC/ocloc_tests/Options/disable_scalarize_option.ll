;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys,pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -ze-opt-large-register-file -igc_opts 'DisableOCLScalarizer=1,EnableVectorizer=0,PrintToConsole=1,PrintAfter=EmitPass'" 2>&1 | FileCheck %s --check-prefixes=CHECK
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -ze-opt-large-register-file -igc_opts 'DisablePHIScalarization=1,EnableVectorizer=0,PrintToConsole=1,PrintAfter=EmitPass'" 2>&1 | FileCheck %s --check-prefixes=CHECK

; Check that phi is not scalarized when DisableOCLScalarizer=1 or DisablePHIScalarization=1 is passed to IGC
; Disabling vectorizer as well in order to check that scalarizer is disabled and not vectorizer transformed the code

; CHECK-LABEL: @foo(
; CHECK:       [[VEC_PHI:%.*]] = phi <8 x [[TYPE:[A-Za-z0-9]+]]>
; CHECK-NOT:   [[SCALAR_FLOAT_PHI:%.*]] = phi float
; CHECK:       ret void


define spir_kernel void @foo(i8 addrspace(1)* %0, i32 %1, i32 %2, i32 %3, i32 %4, i64 %5, <3 x i64> %6, <3 x i64> %7, <3 x i64> %8, i64 %9, i32 %10, i32 %11, i1 %12, i32 %13, i32 %14, i32 %15, i32 %16, <2 x i32> %17, [8 x i32]* %18, <2 x i32> %19, i32* %20, i1 %21, <8 x float> %22, i32 %23, i32 %24, i32 %25, i32 %26, <2 x i32> %27) !intel_reqd_sub_group_size !0 {
  %29 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()
  %30 = shl i32 %1, 2
  br i1 %12, label %.lr.ph, label %._crit_edge

.lr.ph:                                           ; preds = %.lr.ph, %28
  %31 = call spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()
  call spir_func void @_Z52intel_sub_group_2d_block_read_transform_16b_16r16x1cPU3AS1viiiDv2_iPj(i8 addrspace(1)* null, i32 0, i32 0, i32 0, <2 x i32> %17, i32* %20)
  %32 = call spir_func <8 x float> @_Z40intel_sub_group_bf16_bf16_matrix_mad_k16Dv8_sDv8_iDv8_f()
  br i1 %21, label %.lr.ph, label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph, %28
  %_406 = phi <8 x float> [ zeroinitializer, %28 ], [ %22, %.lr.ph ]
  %33 = alloca [8 x i32], i32 0, align 4
  %34 = call spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()
  %35 = shl i32 %2, 2
  %36 = shl i32 %3, 2
  %37 = bitcast [8 x i32]* %33 to <8 x float>*
  store <8 x float> %_406, <8 x float>* %37, align 32
  %38 = insertelement <2 x i32> %17, i32 %30, i32 1
  %39 = bitcast [8 x i32]* %33 to i32*
  call spir_func void @_Z42intel_sub_group_2d_block_write_32b_8r16x1cPU3AS1viiiDv2_iPj(i8 addrspace(1)* %0, i32 %35, i32 %1, i32 %36, <2 x i32> %38, i32* %39)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare spir_func void @_Z52intel_sub_group_2d_block_read_transform_16b_16r16x1cPU3AS1viiiDv2_iPj(i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i32*)

declare spir_func <8 x float> @_Z40intel_sub_group_bf16_bf16_matrix_mad_k16Dv8_sDv8_iDv8_f()

declare spir_func void @_Z42intel_sub_group_2d_block_write_32b_8r16x1cPU3AS1viiiDv2_iPj(i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i32*)

declare spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()

declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()

!0 = !{i32 16}
