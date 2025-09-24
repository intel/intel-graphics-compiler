;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Verify that byte-wise (i8) GEPs with a non-constant index are NOT scalarized
; by the legacy handleGEPInst path. The pass should skip transforming these to
; avoid incorrect handling of inter-lane (unaligned) byte offsets.
; The new algorithm (when enabled) can handle them, but is not used yet.

; CHECK-LABEL: @test(
; CHECK: for.body:
; CHECK: %idx = phi i64 [ 0, %entry ], [ %idx.next, %for.body ]
; CHECK: %src.gep = getelementptr i8, ptr {{%.*}}, i64 %idx
; CHECK: %dst.gep = getelementptr i8, ptr {{%.*}}, i64 %idx
; CHECK: load i8, ptr %src.gep
; CHECK: store i8
; CHECK-NOT: insertelement
; CHECK-NOT: extractelement

define spir_kernel void @test() {
entry:
  %src = alloca [64 x i32], align 4
  %dst = alloca [64 x i32], align 4
  br label %for.body

for.body:                                        ; preds = %entry, %for.body
  %idx = phi i64 [ 0, %entry ], [ %idx.next, %for.body ]
  %src.gep = getelementptr i8, ptr %src, i64 %idx
  %dst.gep = getelementptr i8, ptr %dst, i64 %idx
  %val = load i8, ptr %src.gep, align 1
  store i8 %val, ptr %dst.gep, align 1
  %idx.next = add nuw i64 %idx, 1
  %exitcond = icmp eq i64 %idx.next, 256          ; 256 bytes = 64 * 4
  br i1 %exitcond, label %exit, label %for.body

exit:
  ret void
}

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
