;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; Checks for multiple uses of __spirv_AccessChain function call - load plus store
; it must result in extract and then insert an element to the matrix's slice

; CHECK:  [[SLICE:%.*]] = load <8 x i16>, <8 x i16>* %{{.*}}, align 8
; CHECK:  [[ELEMENT:%.*]] = extractelement <8 x i16> [[SLICE]], i64 4, !joint_matrix_apply
; CHECK:  [[ADD:%.*]] = add i16 [[ELEMENT]], 1
; CHECK:  [[INSERT:%.*]] = insertelement <8 x i16> [[SLICE]], i16 [[ADD]], i64 4
; CHECK:  store <8 x i16> [[INSERT]], <8 x i16>* %{{.*}}, align 8

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"


; Function Attrs: nounwind
define spir_kernel void @_ZTS5logicILm8ELm16EE(i16 addrspace(1)* %arg) {
entry:
  %0 = alloca target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 )
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 ) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__short_3_8_16_0PU3AS1slii(i16 addrspace(1)* %arg, i32 0, i64 64, i32 0)
  store target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 ) %1, target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 )* %0
  %ptr = call spir_func i16 addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR._short_3_8_16_0l(target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 )* %0, i64 4)
  %extract = load i16, i16 addrspace(4)* %ptr
  %add = add i16 %extract, 1
  store i16 %add, i16 addrspace(4)* %ptr
  ret void
}

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 ) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__short_3_8_16_0PU3AS1slii(i16 addrspace(1)* %0, i32 %1, i64 %2, i32 %3)

; Function Attrs: nounwind
declare spir_func i16 addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR._short_3_8_16_0l(target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0 )* %0, i64 %1)

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{void (i16 addrspace(1)*)* @_ZTS5logicILm8ELm16EE, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
