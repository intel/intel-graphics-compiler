;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-16-plus
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -igc_opts 'EnableOpaquePointersBackend=1 ShaderDumpEnable=1, PrintToConsole=1'" -file %t.bc 2>&1 | FileCheck %s

; CHECK: _after_ir_adaptors.ll
; CHECK:  call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(

@.str.6 = internal addrspace(2) constant [72 x i8] c"ERROR FOR_S_INSVIRMEM: Issue allocating chardesc.address in for_concat\0A\00"

declare spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*)

define internal spir_func void @for_concat() #0 {
  %1 = select i1 false, i1 false, i1 true
  br i1 %1, label %9, label %2

2:                                                ; preds = %0
  br label %3

3:                                                ; preds = %2
  br label %4

4:                                                ; preds = %3
  br label %5

5:                                                ; preds = %4
  br label %6

6:                                                ; preds = %5
  %7 = icmp eq i32 0, 0
  %8 = bitcast ptr addrspace(2) null to ptr addrspace(2)
  br label %9

9:                                                ; preds = %6, %0
  %10 = phi ptr addrspace(2) [ @.str.6, %0 ], [ null, %6 ]
  %11 = bitcast ptr addrspace(2) %10 to ptr addrspace(2)
  %12 = call spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(ptr addrspace(2) %11)
  ret void
}

attributes #0 = { noinline }