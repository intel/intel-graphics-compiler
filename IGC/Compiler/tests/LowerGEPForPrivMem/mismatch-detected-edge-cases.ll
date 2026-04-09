;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Edge-case tests for MismatchDetected after refactoring.
; Each function targets a specific code path.

; ---
; 1. Nested arrays [2 x [4 x float]]
;    Peels to float (32 bits). Scalar float store matches, promoted.
; ---
; CHECK-LABEL: @nested_array_peel(
; CHECK: alloca <8 x float>
; CHECK-NOT: alloca [2 x [4 x float]]
; CHECK: ret void
define void @nested_array_peel(float %val) {
  %a = alloca [2 x [4 x float]], align 4, !uniform !4
  %gep = getelementptr [2 x [4 x float]], ptr %a, i32 0, i32 1, i32 2
  store float %val, ptr %gep, align 4
  ret void
}

; ---
; 2. Array of vectors [2 x <4 x float>]
;    Peels through array and vector to float. Scalar store matches, promoted.
; ---
; CHECK-LABEL: @array_of_vectors_peel(
; CHECK: alloca <8 x float>
; CHECK-NOT: alloca [2 x <4 x float>]
; CHECK: ret void
define void @array_of_vectors_peel(float %val) {
  %a = alloca [2 x <4 x float>], align 16, !uniform !4
  %gep = getelementptr [2 x <4 x float>], ptr %a, i32 0, i32 0, i32 1
  store float %val, ptr %gep, align 4
  ret void
}

; ---
; 3. Single-element struct wrapper [4 x { float }]
;    Struct peeling extracts float. Scalar store matches, promoted.
; ---
; CHECK-LABEL: @single_elem_struct_wrapper(
; CHECK: alloca <4 x float>
; CHECK-NOT: alloca [4 x { float }]
; CHECK: ret void
define void @single_elem_struct_wrapper(float %val) {
  %a = alloca [4 x { float }], align 4, !uniform !4
  %gep = getelementptr [4 x { float }], ptr %a, i32 0, i32 2, i32 0
  store float %val, ptr %gep, align 4
  ret void
}

; ---
; 4. Doubly-nested single-element struct [4 x { { float } }]
;    Struct loop peels two levels to reach float. Scalar store matches, promoted.
; ---
; CHECK-LABEL: @double_nested_struct_wrapper(
; CHECK: alloca <4 x float>
; CHECK-NOT: alloca [4 x { { float } }]
; CHECK: ret void
define void @double_nested_struct_wrapper(float %val) {
  %a = alloca [4 x { { float } }], align 4, !uniform !4
  %gep = getelementptr [4 x { { float } }], ptr %a, i32 0, i32 2, i32 0, i32 0
  store float %val, ptr %gep, align 4
  ret void
}

; ---
; 5. Aggregate store [2 x float] into [4 x float]
;    Stored type is an aggregate, rejected by the aggregate guard. Not promoted.
; ---
; CHECK-LABEL: @aggregate_array_store(
; CHECK: alloca [4 x float]
; CHECK-NOT: alloca <4 x float>
; CHECK: ret void
define void @aggregate_array_store([2 x float] %val) {
  %a = alloca [4 x float], align 4, !uniform !4
  store [2 x float] %val, ptr %a, align 4
  ret void
}

; ---
; 6. Dynamic GEP with smaller element type
;    GEP source element is i16 (16 bits), alloca element is i32 (32 bits).
;    Dynamic index plus size mismatch causes rejection. Not promoted.
; ---
; CHECK-LABEL: @dynamic_gep_smaller_type(
; CHECK: alloca [4 x i32]
; CHECK-NOT: alloca <4 x i32>
; CHECK: ret void
define void @dynamic_gep_smaller_type(i64 %idx) {
  %a = alloca [4 x i32], align 4, !uniform !4
  %gep = getelementptr i16, ptr %a, i64 %idx
  %val = load i16, ptr %gep, align 2
  ret void
}

; ---
; 7. Same-size reinterpret via GEP
;    GEP indexes into [4 x i32] but load type is float (both 32 bits).
;    Sizes match, promoted.
; ---
; CHECK-LABEL: @gep_same_size_reinterpret(
; CHECK: alloca <4 x i32>
; CHECK-NOT: alloca [4 x i32]
; CHECK: ret void
define void @gep_same_size_reinterpret(ptr %output) {
  %a = alloca [4 x i32], align 4, !uniform !4
  %gep = getelementptr [4 x i32], ptr %a, i32 0, i32 1
  %val = load float, ptr %gep, align 4
  store float %val, ptr %output, align 4
  ret void
}

; ---
; 8. Size mismatch via GEP
;    GEP element is i32 (32 bits) but load type is i64 (64 bits). Not promoted.
; ---
; CHECK-LABEL: @gep_size_mismatch_load_wider(
; CHECK: alloca [4 x i32]
; CHECK-NOT: alloca <4 x i32>
; CHECK: ret void
define void @gep_size_mismatch_load_wider(ptr %output) {
  %a = alloca [4 x i32], align 4, !uniform !4
  %gep = getelementptr [4 x i32], ptr %a, i32 0, i32 0
  %val = load i64, ptr %gep, align 4
  store i64 %val, ptr %output, align 8
  ret void
}

; ---
; 9. Direct load, same-size reinterpret
;    [4 x float] loaded as i32 (both 32 bits). Sizes match, promoted.
; ---
; CHECK-LABEL: @direct_load_same_size_reinterpret(
; CHECK: alloca <4 x float>
; CHECK-NOT: alloca [4 x float]
; CHECK: ret void
define void @direct_load_same_size_reinterpret(ptr %output) {
  %a = alloca [4 x float], align 4, !uniform !4
  %val = load i32, ptr %a, align 4
  store i32 %val, ptr %output, align 4
  ret void
}

; ---
; 10. Direct load, size mismatch
;     [4 x float] loaded as i64 (64 != 32). Mismatch, not promoted.
; ---
; CHECK-LABEL: @direct_load_size_mismatch(
; CHECK: alloca [4 x float]
; CHECK-NOT: alloca <4 x float>
; CHECK: ret void
define void @direct_load_size_mismatch(ptr %output) {
  %a = alloca [4 x float], align 4, !uniform !4
  %val = load i64, ptr %a, align 4
  store i64 %val, ptr %output, align 8
  ret void
}

; ---
; 11. Multi-field struct [4 x { float, float }]
;     Struct has two fields so peeling stops. IsNativeType rejects it. Not promoted.
; ---
%Pair = type { float, float }

; CHECK-LABEL: @multi_field_struct_not_promoted(
; CHECK: alloca [4 x %Pair]
; CHECK-NOT: alloca <
; CHECK: ret void
define void @multi_field_struct_not_promoted(float %val) {
  %a = alloca [4 x %Pair], align 4, !uniform !4
  %gep = getelementptr [4 x %Pair], ptr %a, i32 0, i32 1, i32 0
  store float %val, ptr %gep, align 4
  ret void
}

; ---
; 12. Struct-array-struct-array nesting [2 x { [2 x { float }] }]
;     Peels: array, single-element struct, array, single-element struct to float.
;     Total elements = 2*2 = 4. Scalar float store matches, promoted.
; ---
; CHECK-LABEL: @struct_array_struct_array_nesting(
; CHECK: alloca <4 x float>
; CHECK-NOT: alloca [2 x
; CHECK: ret void
define void @struct_array_struct_array_nesting(float %val) {
  %a = alloca [2 x { [2 x { float }] }], align 4, !uniform !4
  %gep = getelementptr [2 x { [2 x { float }] }], ptr %a, i32 0, i32 1, i32 0, i32 0, i32 0
  store float %val, ptr %gep, align 4
  ret void
}

!igc.functions = !{!0, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}

!0 = !{ptr @nested_array_peel, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
!5 = !{ptr @array_of_vectors_peel, !1}
!6 = !{ptr @single_elem_struct_wrapper, !1}
!7 = !{ptr @double_nested_struct_wrapper, !1}
!8 = !{ptr @aggregate_array_store, !1}
!9 = !{ptr @dynamic_gep_smaller_type, !1}
!10 = !{ptr @gep_same_size_reinterpret, !1}
!11 = !{ptr @gep_size_mismatch_load_wider, !1}
!12 = !{ptr @direct_load_same_size_reinterpret, !1}
!13 = !{ptr @direct_load_size_mismatch, !1}
!14 = !{ptr @multi_field_struct_not_promoted, !1}
!15 = !{ptr @struct_array_struct_array_nesting, !1}
