;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; RUN: igc_opt -platformpvc --opaque-pointers -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; Checks if unused __spirv_AccessChain function call is removed and doesn't
; cause trouble

; CHECK-NOT: call spir_func ptr addrspace(4) @_Z19__spirv_AccessChain

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @_ZTS5logicILm8ELm16EE() {
entry:
  %0 = alloca target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 0 )
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 0 ) @_Z26__spirv_CompositeConstructf(float 0.0)
  store target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 0 ) %1, ptr %0
  %call = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__float_3_8_16_0l(ptr %0, i64 4)
  ret void
}

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 0 ) @_Z26__spirv_CompositeConstructf(float %0)

; Function Attrs: nounwind
declare spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__float_3_8_16_0l(ptr %0, i64 %1)

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{ptr @_ZTS5logicILm8ELm16EE, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
