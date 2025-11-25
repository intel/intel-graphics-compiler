;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;

; CHECK: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 5}
; CHECK: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 6}

%struct.__generated_KernelArgWithPtr = type { [2 x ptr addrspace(1)], i32, i32 }
%struct.KernelArgWithPtr = type { [2 x ptr addrspace(4)], i32, i32 }

; Function Attrs: convergent nounwind
define spir_kernel void @test(
ptr nocapture readonly byval(%struct.__generated_KernelArgWithPtr) align 8 %__arg_kArg, i64 %_arg_randIndex, <8 x i32> %r0, <3 x i32> %globalOffset, ptr %privateBase, i64 %const_reg_qword, i64 %const_reg_qword1, i32 %const_reg_dword, i32 %const_reg_dword2) #0 {
entry:
  %agg.tmp1 = alloca %struct.KernelArgWithPtr, align 8

  store i64 %const_reg_qword, ptr %agg.tmp1, align 8

  %__arg_kArg_alloca.sroa.2.0..sroa_idx3 = getelementptr inbounds %struct.KernelArgWithPtr, ptr %agg.tmp1, i64 0, i32 0, i64 1
  store i64 %const_reg_qword1, ptr %__arg_kArg_alloca.sroa.2.0..sroa_idx3, align 8

  %__arg_kArg_alloca.sroa.3.0..sroa_idx4 = getelementptr inbounds %struct.KernelArgWithPtr, ptr %agg.tmp1, i64 0, i32 1
  store i32 %const_reg_dword, ptr %__arg_kArg_alloca.sroa.3.0..sroa_idx4, align 8

  %__arg_kArg_alloca.sroa.4.0..sroa_idx5 = getelementptr inbounds %struct.KernelArgWithPtr, ptr %agg.tmp1, i64 0, i32 2
  store i32 %const_reg_dword2, ptr %__arg_kArg_alloca.sroa.4.0..sroa_idx5, align 4

  %add.i = add nsw i32 %const_reg_dword, %const_reg_dword2

  %arrayidx.i = getelementptr inbounds ptr addrspace(4), ptr %agg.tmp1, i64 %_arg_randIndex

  %load = load ptr addrspace(4), ptr %arrayidx.i, align 8
  %arrayidx8.i = getelementptr inbounds i32, ptr addrspace(4) %load, i64 5
  %spaceCast = addrspacecast ptr addrspace(4) %arrayidx8.i to ptr addrspace(1)
  store i32 15, ptr addrspace(1) %spaceCast, align 4

  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
