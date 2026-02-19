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
;       float a;
;       double b;
;       float c;
;     };
;     kernel void test(global struct Foo* p, int n) {
;       for (int i = 0; i < n; ++i) {
;         p->a += i;
;         p->c += i;
;       }
;     }
;
; GEPs to non-array type in aggregate type. Don't optimize them.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

%struct.Foo = type { float, double, float }

define spir_kernel void @test(%struct.Foo addrspace(1)* %p, i32 %n) #0 {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK-NOT:     getelementptr
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add4, %for.body ]
; CHECK:         %conv = sitofp i32 %i.02 to float
; CHECK:         %a = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i32 0, i32 0
; CHECK:         %0 = load float, float addrspace(1)* %a, align 4
; CHECK:         %add = fadd float %0, %conv
; CHECK:         store float %add, float addrspace(1)* %a, align 4
; CHECK:         %conv1 = sitofp i32 %i.02 to float
; CHECK:         %c = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i32 0, i32 2
; CHECK:         %1 = load float, float addrspace(1)* %c, align 4
; CHECK:         %add2 = fadd float %1, %conv1
; CHECK:         %add4 = add nuw nsw i32 %i.02, 2
; CHECK:         %cmp = icmp slt i32 %add4, %n
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add4, %for.body ]
  %conv = sitofp i32 %i.02 to float
  %a = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i32 0, i32 0
  %0 = load float, float addrspace(1)* %a, align 4
  %add = fadd float %0, %conv
  store float %add, float addrspace(1)* %a, align 4
  %conv1 = sitofp i32 %i.02 to float
  %c = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %p, i32 0, i32 2
  %1 = load float, float addrspace(1)* %c, align 4
  %add2 = fadd float %1, %conv1
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
