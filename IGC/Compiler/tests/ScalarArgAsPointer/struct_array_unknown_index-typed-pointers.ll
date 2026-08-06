;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; SYCL struct contains two pointers to global memory, but they are placed inside array:
;
;   struct MyArg {
;     float padding1;
;     struct {
;       int padding;
;       int* ptr;
;     } structs[2];
;     float padding2;
;   };
;
; SYCL kernel uses one of these two pointers, but array index is unknown at compilation time:
;   *myarg.structs[randIndex].ptr = 39;
;
; If pass can't deduce exact pointer in array, it has to mark all pointers in array. This
; produces false positives, but it is acceptable outcome.
;
; Note: IR below already shows decomposed struct.
;
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 2}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 3}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 4}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 5}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 6}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 7}

%class.anon = type <{ %struct.MyArg, i32, [4 x i8] }>
%struct.MyArg = type { float, [2 x %struct.anon], float }
%struct.anon = type { i32, i32 addrspace(4)* }

define spir_kernel void @test(%struct.MyArg* byval(%struct.MyArg) %_arg_myarg, i32 %_arg_randIndex, float %const_reg_fp32, i32 %const_reg_dword, i64 %const_reg_qword, i32 %const_reg_dword1, i64 %const_reg_qword2, float %const_reg_fp323) #0 {
entry:

  ; Kernel allocates local copy of struct kernel argument using implicit arguments. Because array index
  ; is unknown at compilation time, this will not be optimized out.
  %__SYCLKernel = alloca %class.anon, align 8
  %_myarg_idx = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 0
  store float %const_reg_fp32, float* %_myarg_idx, align 8
  %_myarg_idx5 = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 1, i64 0, i32 0
  store i32 %const_reg_dword, i32* %_myarg_idx5, align 8
  %_myarg_idx7 = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 1, i64 0, i32 1
  %_myarg_cast7 = bitcast i32 addrspace(4)** %_myarg_idx7 to i64*
  store i64 %const_reg_qword, i64* %_myarg_cast7, align 8
  %_myarg_idx8 = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 1, i64 1, i32 0
  store i32 %const_reg_dword1, i32* %_myarg_idx8, align 8
  %_myarg_idx10 = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 1, i64 1, i32 1
  %_myarg_cast10 = bitcast i32 addrspace(4)** %_myarg_idx10 to i64*
  store i64 %const_reg_qword2, i64* %_myarg_cast10, align 8
  %_myarg_idx11 = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 2
  store float %const_reg_fp323, float* %_myarg_idx11, align 8

  ; Access to global memory via pointer in struct.
  %idxprom.i = sext i32 %_arg_randIndex to i64
  %ptr.i = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i64 0, i32 0, i32 1, i64 %idxprom.i, i32 1
  %0 = load i32 addrspace(4)*, i32 addrspace(4)** %ptr.i, align 8
  %1 = addrspacecast i32 addrspace(4)* %0 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %1, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (%struct.MyArg*, i32, float, i32, i64, i32, i64, float)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
