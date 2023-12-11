;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%struct_without_bools = type { i8 }
%struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }

; CHECK:        %struct_without_bools = type { i8 }
; CHECK:        %struct = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }


define spir_func void @fun_struct_without_bools(i1 %input1, %struct_without_bools %input2) {
  ret void
}

; CHECK:        define spir_func void @fun_struct_without_bools(i8 %input1, %struct_without_bools %input2)


define spir_func i1 @callee0(i1 %input) {
  ret i1 %input
}

; CHECK:        define spir_func i8 @callee0(i8 %input)
; CHECK-NEXT:   ret i8 %input

define spir_func i1 @callee1(%struct addrspace(1)* %input_struct)
{
  %1 = load %struct, %struct addrspace(1)* %input_struct
  ret i1 true
}

; CHECK:        define spir_func i8 @callee1(%struct addrspace(1)* %input_struct)
; CHECK-NEXT:   %1 = load %struct, %struct addrspace(1)* %input_struct
; CHECK-NEXT:   ret i8 1

define spir_func void @caller(i1 %input, %struct addrspace(1)* %input_struct) {
  %1 = call i1 @callee0(i1 false)
  %2 = call i1 @callee0(i1 true)
  %3 = call i1 @callee0(i1 %input)
  %4 = call i1 @callee1(%struct addrspace(1)* %input_struct)
  ret void
}

; CHECK:        define spir_func void @caller(i8 %input, %struct addrspace(1)* %input_struct)
; CHECK-NEXT:   %1 = call i8 @callee0(i8 0)
; CHECK-NEXT:   %2 = call i8 @callee0(i8 1)
; CHECK-NEXT:   %3 = call i8 @callee0(i8 %input)
; CHECK-NEXT:   %4 = call i8 @callee1(%struct addrspace(1)* %input_struct)
; CHECK-NEXT:   ret void
