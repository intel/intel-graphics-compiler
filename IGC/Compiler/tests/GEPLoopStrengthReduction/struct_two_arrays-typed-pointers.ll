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
;       int baz[100];
;     };
;     kernel void test(global struct Foo* p, int n) {
;       for (int i = 0; i < n; ++i) {
;         p->baz[i] = p->bar[i];
;       }
;     }
;
; Two unrelated GEP with multiple indices. Reduce them to two new
; induction variables. Output:
;
;     kernel void test(global struct Foo* p, int n) {
;       global int* tmp1 = p->bar;
;       global int* tmp2 = p->baz;
;       for (int i = 0; i < n; ++i, ++tmp1, ++tmp2) {
;         *tmp2 = *tmp1;
;       }
;     }

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

%struct.Foo = type { [100 x i32], [100 x i32] }

define spir_kernel void @test(%struct.Foo addrspace(1)* %p, i32 %n) {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP1_PHI1:%.*]] = getelementptr %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 0, i64 0
; CHECK:         [[GEP2_PHI1:%.*]] = getelementptr %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 1, i64 0
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP2:%.*]] = phi i32 addrspace(1)* [ [[GEP2_PHI1]], %for.body.lr.ph ], [ [[GEP2_PHI2:%.*]], %for.body ]
; CHECK:         [[GEP1:%.*]] = phi i32 addrspace(1)* [ [[GEP1_PHI1]], %for.body.lr.ph ], [ [[GEP1_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p
; CHECK:         [[LOAD:%.*]] = load i32, i32 addrspace(1)* [[GEP1]], align 4
; CHECK-NOT:     getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p
; CHECK:         store i32 [[LOAD]], i32 addrspace(1)* [[GEP2]], align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         [[GEP1_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP1]], i64 1
; CHECK:         [[GEP2_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP2]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %idxprom = zext i32 %i.02 to i64
  %arrayidx = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 0, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i64 0, i32 1, i64 %idxprom
  store i32 %0, i32 addrspace(1)* %arrayidx2, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
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
