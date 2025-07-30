;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, pvc-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -igc_opts 'EnableOpaquePointersBackend=1, ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.zeinfo | FileCheck %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.zeinfo | FileCheck %s

; CHECK-LABEL: - name: kernel
; CHECK: execution_env:
; CHECK:   indirect_stateless_count: 2

define dllexport spir_kernel void @kernel(i32 addrspace(1)* addrspace(1)* "VCArgumentIOKind"="0" %pptr, i64 "VCArgumentIOKind"="0" %i, i64 "VCArgumentIOKind"="0" %j) #0 {
  %pptri = getelementptr i32 addrspace(1) *, i32 addrspace(1)* addrspace (1)* %pptr, i64 %i
  %ptr = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(1)* %pptri
  %ptrj = getelementptr i32, i32 addrspace(1)* %ptr, i64 %j
  %val = load i32, i32 addrspace(1)* %ptrj
  %add = add i32 %val, 1
  store i32 %add, i32 addrspace(1)* %ptrj
  ret void
}

attributes #0 = { nounwind "VCFunction" "VCNamedBarrierCount"="0" "VCSLMSize"="0" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
