;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o - | FileCheck %s

%simple_struct = type { i1, i32, i1 }
%complex_struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }

; CHECK:        [[NEW_SIMPLE_STRUCT:%simple_struct.[0-9]+]] = type { i8, i32, i8 }
; CHECK:        [[NEW_COMPLEX_STRUCT:%complex_struct.[0-9]+]] = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }


define spir_func i1 @foo(i1 %input) {
  ret i1 %input
}

; CHECK:        define spir_func i8 @foo(i8 %input)
; CHECK-NEXT:   ret i8 {{%[a-zA-Z0-9]+}}


define spir_func void @main(i1 %input) {
  %result_false = call i1 @foo(i1 false)
  %result_true = call i1 @foo(i1 true)
  %result = call i1 @foo(i1 %input)
  ret void
}

; CHECK:        define spir_func void @main(i8 %input)
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = call i8 @foo(i8 0)
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = call i8 @foo(i8 1)
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = call i8 @foo(i8 {{%[a-zA-Z0-9]+}})
; CHECK-NEXT:   ret void


define spir_func void @function_vector_arg(<4 x i1> %input) {
  ret void
}

; CHECK:        define spir_func void @function_vector_arg(<4 x i8> %input)


define spir_func void @function_pointer_arg(i1* %input) {
  ret void
}

; CHECK:        define spir_func void @function_pointer_arg(i8* %input)


define spir_func void @function_array_arg([4 x i1] %input) {
  ret void
}

; CHECK:        define spir_func void @function_array_arg([4 x i8] %input)


define spir_func void @function_struct_arg(%simple_struct %input) {
  ret void
}

; CHECK:        define spir_func void @function_struct_arg([[NEW_SIMPLE_STRUCT]] %input)


define spir_func void @function_rec_test(%complex_struct %input) {
  ret void
}

; CHECK:        define spir_func void @function_rec_test([[NEW_COMPLEX_STRUCT]] %input)
