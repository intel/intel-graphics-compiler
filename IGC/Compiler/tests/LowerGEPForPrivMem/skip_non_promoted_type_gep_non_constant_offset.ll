;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Verify that non-promoted type GEPs with a non-constant index won't use SOA layout.
; PrivateMemoryResolution pass then assumes the alloca element type will be the same
; as the element in GEP. This triggers assert in that pass and leads to miscompilations.

; CHECK-LABEL: @test(
; CHECK: for.body:
; CHECK: %idx = phi i64 [ 0, %entry ], [ %idx.next, %for.body ]
; CHECK: %src.gep = getelementptr <8 x i32>, ptr {{%.*}}, i64 %idx
; CHECK: %dst.gep = getelementptr <8 x i32>, ptr {{%.*}}, i64 %idx
; CHECK: load <8 x i32>, ptr %src.gep
; CHECK: store <8 x i32>
; CHECK-NOT: insertelement
; CHECK-NOT: extractelement
; CHECK: CannotUseSOALayout

define spir_kernel void @test() {
entry:
  %src = alloca [128 x i8], align 4
  %dst = alloca [128 x i8], align 4
  br label %for.body

for.body:
  %idx = phi i64 [ 0, %entry ], [ %idx.next, %for.body ]
  %src.gep = getelementptr <8 x i32>, ptr %src, i64 %idx
  %dst.gep = getelementptr <8 x i32>, ptr %dst, i64 %idx
  %val = load <8 x i32>, ptr %src.gep, align 4
  store <8 x i32> %val, ptr %dst.gep, align 4
  %idx.next = add i64 %idx, 1
  %exitcond = icmp ult i64 %idx.next, 4
  br i1 %exitcond, label %exit,label %for.body

exit:
  ret void
}

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
