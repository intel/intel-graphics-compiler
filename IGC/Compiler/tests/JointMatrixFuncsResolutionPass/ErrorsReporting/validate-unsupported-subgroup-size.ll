;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - forcing unsupported subgroup size is reported
; as an error instead of crashing the compiler.
; ------------------------------------------------

; CHECK: error: Sub group size 16 is forced by attribute but not supported by Joint Matrix on this platform.

define spir_kernel void @test(ptr addrspace(1) %dst) {
  %m = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) @_Z26__spirv_CompositeConstructf(float 0.000000e+00)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fPU3AS144__spirv_CooperativeMatrixKHR__float_3_8_16_2ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) %m, i32 0, i64 16, i32 0)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) @_Z26__spirv_CompositeConstructf(float)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fPU3AS144__spirv_CooperativeMatrixKHR__float_3_8_16_2ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMD", !5, !4}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
