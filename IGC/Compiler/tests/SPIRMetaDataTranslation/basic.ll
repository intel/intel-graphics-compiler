;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-spir-metadata-translation -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; SPIRMetaDataTranslation
; ------------------------------------------------

; Test checks that SPIRMetaData is propagated to IGCMetaData

declare spir_kernel void @test_spir(i64 addrspace(1)*)

; CHECK-DAG: {!"DenormsAreZero", i1 true}
; CHECK-DAG: {!"CorrectlyRoundedDivSqrt", i1 true}
; CHECK-DAG: {!"OptDisable", i1 true}
; CHECK-DAG: {!"MadEnable", i1 true}
; CHECK-DAG: {!"NoSignedZeros", i1 true}
; CHECK-DAG: {!"UnsafeMathOptimizations", i1 true}
; CHECK-DAG: {!"FiniteMathOnly", i1 true}
; CHECK-DAG: {!"FastRelaxedMath", i1 true}
; CHECK-DAG: {!"DashGSpecified", i1 true}
; CHECK-DAG: {!"RelaxedBuiltins", i1 true}
; CHECK-DAG: {!"MatchSinCosPi", i1 true}
; CHECK-DAG: {!"FuncMDMap[0]", void (i64 addrspace(1)*)* @test_spir}
; CHECK-DAG: {!"thread_group_size", i32 1, i32 1, i32 16}
; CHECK-DAG: {!"thread_group_size_hint", i32 1, i32 1, i32 4}
; CHECK-DAG: {!"function_type", i32 0}
; CHECK-DAG: {!"sub_group_size", i32 16}
; CHECK-DAG: {!"m_OpenCLArgAddressSpaces", [[AS_VEC:![0-9]*]]}
; CHECK-DAG: [[AS_VEC]] = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
; CHECK-DAG: {!"m_OpenCLArgAccessQualifiers", [[ACQ_VEC:![0-9]*]]}
; CHECK-DAG: [[ACQ_VEC]] = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
; CHECK-DAG: {!"m_OpenCLArgTypes", [[ATYPE_VEC:![0-9]*]]}
; CHECK-DAG: [[ATYPE_VEC]] = !{!"m_OpenCLArgTypesVec[0]", !"long*"}
; CHECK-DAG: {!"m_OpenCLArgBaseTypes", [[BTYPE_VEC:![0-9]*]]}
; CHECK-DAG: [[BTYPE_VEC]] = !{!"m_OpenCLArgBaseTypesVec[0]", !"long*"}
; CHECK-DAG: {!"m_OpenCLArgTypeQualifiers", [[ATYPEQ_VEC:![0-9]*]]}
; CHECK-DAG: [[ATYPEQ_VEC]] = !{!"m_OpenCLArgTypeQualifiersVec[0]", !"volatile"}
; CHECK-DAG: {!"m_OpenCLArgNames", [[ANAME_VEC:![0-9]*]]}
; CHECK-DAG: [[ANAME_VEC]] = !{!"m_OpenCLArgNamesVec[0]", !"dst"}

!opencl.kernels = !{!0}
!opencl.compiler.options = !{!11}

!0 = !{void (i64 addrspace(1)*)* @test_spir, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10}
!1 = !{!"kernel_arg_addr_space", i32 1}
!2 = !{!"kernel_arg_access_qual", !"none"}
!3 = !{!"kernel_arg_type", !"long*"}
!4 = !{!"kernel_arg_type_qual", !"volatile"}
!5 = !{!"kernel_arg_base_type", !"long*"}
!6 = !{!"kernel_arg_name", !"dst"}
!7 = !{!"reqd_work_group_size", i32 1, i32 1, i32 16}
!8 = !{!"work_group_size_hint", i32 1, i32 1, i32 4}
!9 = !{!"vec_type_hint", <4 x float> undef, i32 0}
!10 = !{!"intel_reqd_sub_group_size", i32 16}
!11 = !{!"-cl-std=CL2.0", !"-cl-opt-disable", !"-g", !"-denorms-are-zero", !"-fp32-correctly-rounded-divide-sqrt", !"-mad-enable", !"-no-signed-zeros", !"-unsafe-math-optimizations", !"-finite-math-only", !"-fast-relaxed-math", !"-relaxed-builtins", !"-match-sincospi"}
