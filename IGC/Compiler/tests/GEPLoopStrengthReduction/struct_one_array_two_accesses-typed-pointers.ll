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
; Input:
;
;     struct Foo {
;       int bar[100];
;     };
;     kernel void test(global struct Foo* p, int n) {
;       for (int i = 0; i < n; i+=2) {
;         p->bar[i+1] = p->bar[i];
;       }
;     }
;
; GEP has multiple indices, but only the last one is variable. Change GEP
; into pointer to accessed type and add it as induction variable. Output:
;
;     kernel void test(global struct Foo* p, int n) {
;       global int* tmp = p->bar;
;       for (int i = 0; i < n; i+=2, tmp+=2) {
;         *(tmp+1) = *tmp;
;       }
;     }

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

%struct.Foo = type { [100 x i32] }

define spir_kernel void @test(%struct.Foo addrspace(1)* %p, i32 %n) {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 0, i64 0
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP0:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add4, %for.body ]
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p
; CHECK:         [[LOAD:%.*]] = load i32, i32 addrspace(1)* [[GEP0]], align 4
; CHECK-NOT:     or {i32|i64}
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p
; CHECK:         [[GEP1:%.*]] = getelementptr i32, i32 addrspace(1)* [[GEP0]], i64 1
; CHECK:         store i32 [[LOAD]], i32 addrspace(1)* [[GEP1]], align 4
; CHECK:         %add4 = add nuw nsw i32 %i.02, 2
; CHECK:         %cmp = icmp slt i32 %add4, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP0]], i64 2
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add4, %for.body ]
  %idxprom = zext i32 %i.02 to i64
  %arrayidx = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 0, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = or i32 %i.02, 1
  %idxprom2 = zext i32 %add to i64
  %arrayidx3 = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 0, i64 %idxprom2
  store i32 %0, i32 addrspace(1)* %arrayidx3, align 4
  %add4 = add nuw nsw i32 %i.02, 2
  %cmp = icmp slt i32 %add4, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (%struct.Foo addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
