;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s
;
; Index depends on value recalculated on every loop iteration to unknown value.
; Starting pointer can't be reduced to loop's preheader, don't change anything.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

; Function Attrs: nounwind
declare spir_func i64 @random()

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK-NOT:     getelementptr i32, i32 addrspace(1)* %p
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK-NOT:     phi i32 addrspace(1)*
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         %zext = zext i32 %i.02 to i64
; CHECK:         %call = call spir_func i64 @random()
; CHECK:         %add = add nsw i64 %zext, %call
; CHECK:         %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %add
; CHECK:         store i32 %i.02, i32 addrspace(1)* %arrayidx, align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %zext = zext i32 %i.02 to i64
  %call = call spir_func i64 @random()
  %add = add nsw i64 %zext, %call
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %add
  store i32 %i.02, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
