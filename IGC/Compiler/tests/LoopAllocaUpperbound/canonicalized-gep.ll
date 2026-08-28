;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-loop-alloca-upperbound -S %s | FileCheck %s
; InstCombine may strip a GEP's leading zero index. Recover the array bound
; from the alloca when the remaining GEP indexes its element type.

%struct.Color = type { float, float, float }

define spir_kernel void @test_canonicalized_gep(i32 %n) {
; CHECK-LABEL: @test_canonicalized_gep(
; CHECK:         icmp slt i32 {{%.*}}, 8
;
bb:
  %i = alloca [8 x %struct.Color], align 4
  %guard = icmp sgt i32 %n, 0
  br i1 %guard, label %loop.lr.ph, label %exit

loop.lr.ph:                                       ; preds = %bb
  br label %loop

loop:                                             ; preds = %loop.lr.ph, %loop
  %iv = phi i32 [ 0, %loop.lr.ph ], [ %iv.next, %loop ]
  %idx = zext i32 %iv to i64
  %gep = getelementptr inbounds %struct.Color, ptr %i, i64 %idx
  store float 0.000000e+00, ptr %gep, align 4
  %iv.next = add nuw nsw i32 %iv, 1
  %cmp = icmp slt i32 %iv.next, %n
  br i1 %cmp, label %loop, label %loop.exit

loop.exit:                                        ; preds = %loop
  br label %exit

exit:                                             ; preds = %loop.exit, %bb
  ret void
}

; Use the outer dimension of a multidimensional alloca.
define spir_kernel void @test_multidim_outer_index(i32 %n) {
; CHECK-LABEL: @test_multidim_outer_index(
; CHECK:         icmp slt i32 {{%.*}}, 4
;
bb:
  %a = alloca [4 x [8 x i32]], align 4
  %guard = icmp sgt i32 %n, 0
  br i1 %guard, label %loop.lr.ph, label %exit

loop.lr.ph:                                       ; preds = %bb
  br label %loop

loop:                                             ; preds = %loop.lr.ph, %loop
  %iv = phi i32 [ 0, %loop.lr.ph ], [ %iv.next, %loop ]
  %idx = zext i32 %iv to i64
  %gep = getelementptr inbounds [8 x i32], ptr %a, i64 %idx
  store i32 0, ptr %gep, align 4
  %iv.next = add nuw nsw i32 %iv, 1
  %cmp = icmp slt i32 %iv.next, %n
  br i1 %cmp, label %loop, label %loop.exit

loop.exit:                                        ; preds = %loop
  br label %exit

exit:                                             ; preds = %loop.exit, %bb
  ret void
}

; Do not treat a byte offset as an array element index.
define spir_kernel void @test_byte_scaled_gep_not_bounded(i32 %n) {
; CHECK-LABEL: @test_byte_scaled_gep_not_bounded(
; CHECK-NOT:     icmp slt i32 {{%.*}}, 8
;
bb:
  %i = alloca [8 x %struct.Color], align 4
  %guard = icmp sgt i32 %n, 0
  br i1 %guard, label %loop.lr.ph, label %exit

loop.lr.ph:                                       ; preds = %bb
  br label %loop

loop:                                             ; preds = %loop.lr.ph, %loop
  %iv = phi i32 [ 0, %loop.lr.ph ], [ %iv.next, %loop ]
  %idx = zext i32 %iv to i64
  %gep = getelementptr inbounds i8, ptr %i, i64 %idx
  store float 0.000000e+00, ptr %gep, align 4
  %iv.next = add nuw nsw i32 %iv, 1
  %cmp = icmp slt i32 %iv.next, %n
  br i1 %cmp, label %loop, label %loop.exit

loop.exit:                                        ; preds = %loop
  br label %exit

exit:                                             ; preds = %loop.exit, %bb
  ret void
}
