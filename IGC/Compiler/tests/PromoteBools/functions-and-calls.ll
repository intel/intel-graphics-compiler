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
; CHECK:        [[NEW_STRUCT:%struct.[0-9]+]] = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }


define spir_func void @fun_struct_without_bools(i1 %input1, %struct_without_bools %input2) {
  ret void
}

; CHECK:        define spir_func void @fun_struct_without_bools(i8 %input1, %struct_without_bools %input2)


define spir_func i1 @callee(i1 %input) {
  ret i1 %input
}

; CHECK:        define spir_func i8 @callee(i8 %input)
; CHECK-NEXT:   ret i8 {{%[a-zA-Z0-9]+}}


define spir_func void @caller(i1 %input, %struct %input_struct) {
  %result_false = call i1 @callee(i1 false)
  %result_true = call i1 @callee(i1 true)
  %result = call i1 @callee(i1 %input)
  ret void
}

; CHECK:        define spir_func void @caller(i8 %input, [[NEW_STRUCT]] %input_struct)
; CHECK-NEXT:   %result_false = call i8 @callee(i8 0)
; CHECK-NEXT:   %result_true = call i8 @callee(i8 1)
; CHECK-NEXT:   %result = call i8 @callee(i8 {{%[a-zA-Z0-9]+}})
; CHECK-NEXT:   ret void
