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
; Pass targets only accesses unique for loop. If loop's induction variable is used
; after loop to access the same pointer, don't optimize it.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

%struct.Foo = type { float, float }

define spir_kernel void @test(%struct.Foo addrspace(1)* %p, i32 %n) {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK-NOT:     getelementptr
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         %idxprom3 = phi i64 [ 0, %for.body.lr.ph ], [ %idxprom, %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         %ptmp = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 %idxprom3
; CHECK:         %a = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %ptmp, i64 0, i32 0
; CHECK:         %0 = load float, float addrspace(1)* %a, align 4
; CHECK:         %add = fadd float %0, 1.000000e+00
; CHECK:         store float %add, float addrspace(1)* %a, align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         %idxprom = zext i32 %inc to i64
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %idxprom3 = phi i64 [ 0, %for.body.lr.ph ], [ %idxprom, %for.body ]
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %ptmp = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 %idxprom3
  %a = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %ptmp, i64 0, i32 0
  %0 = load float, float addrspace(1)* %a, align 4
  %add = fadd float %0, 1.000000e+00
  store float %add, float addrspace(1)* %a, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  %idxprom = zext i32 %inc to i64
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  %idxprom.lcssa4 = phi i64 [ %idxprom3, %for.body ]
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  %idxprom.lcssa = phi i64 [ %idxprom.lcssa4, %for.cond.for.end_crit_edge ], [ 0, %entry ]
  %b = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 %idxprom.lcssa, i32 1
  %1 = load float, float addrspace(1)* %b, align 4
  %add3 = fadd float %1, 1.000000e+00
  store float %add3, float addrspace(1)* %b, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (%struct.Foo addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
