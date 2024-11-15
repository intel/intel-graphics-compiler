;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize --opaque-pointers -S < %s | FileCheck %s
; REQUIRES: llvm-14-plus, opaque-ptr-fix
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------
; The pass should break vector operation into many scalar operations

; add checks when the pass will support opaque pointers (and remove "opaque-ptr-fix")
; ------------------------------------------------

define double @basic(<2 x ptr> %pointers) {
  %pointer_to_double = getelementptr double, <2 x ptr> %pointers, i32 1

  %ptr0 = extractelement <2 x ptr> %pointer_to_double, i32 0
  %ptr1 = extractelement <2 x ptr> %pointer_to_double, i32 1
  %val0 = load double, ptr %ptr0
  %val1 = load double, ptr %ptr1
  %return = fadd double %val0, %val1
  ret double %return
}

define double @should_work_with_vector_of_indices(<2 x ptr> %pointers) {
  %pointers_to_double = getelementptr double, <2 x ptr> %pointers, <2 x i32> <i32 0, i32 1>

  %ptr0 = extractelement <2 x ptr> %pointers_to_double, i32 0
  %ptr1 = extractelement <2 x ptr> %pointers_to_double, i32 1
  %val0 = load double, ptr %ptr0
  %val1 = load double, ptr %ptr1
  %return = fadd double %val0, %val1
  ret double %return
}

define i64 @should_work_with_different_value_type(<2 x ptr> %pointers) {
  %pointer_to_i64 = getelementptr i64, <2 x ptr> %pointers, i32 1

  %ptr0 = extractelement <2 x ptr> %pointer_to_i64, i32 0
  %ptr1 = extractelement <2 x ptr> %pointer_to_i64, i32 1
  %val0 = load i64, ptr %ptr0
  %val1 = load i64, ptr %ptr1
  %return = add i64 %val0, %val1
  ret i64 %return
}

define double @should_work_with_larger_vector_size(<16 x ptr> %pointers) {
  %pointer_to_double = getelementptr double, <16 x ptr> %pointers, i32 1

  %ptr0 = extractelement <16 x ptr> %pointer_to_double, i32 0
  %ptr1 = extractelement <16 x ptr> %pointer_to_double, i32 1
  %val0 = load double, ptr %ptr0
  %val1 = load double, ptr %ptr1
  %return = fadd double %val0, %val1
  ret double %return
}

%some_type = type {i64, i32}

define i64 @should_not_scalarize_with_more_then_index(ptr %pointer) {
  %pointer_to_int = getelementptr %some_type, ptr %pointer, i32 0, i32 0

  %val0 = load i64, ptr %pointer_to_int
  %return = add i64 %val0, %val0
  ret i64 %return
}

define i64 @should_scalarize_only_vectors(ptr %pointer) {
  %pointer_some_type = getelementptr %some_type, ptr %pointer, i32 1

  %val = load %some_type, ptr %pointer_some_type

  %val0 = extractvalue %some_type %val, 0
  %val1 = extractvalue %some_type %val, 0
  %return = add i64 %val0, %val1
  ret i64 %return
}
