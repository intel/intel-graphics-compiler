;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
;
; Verify that incoming edges bringing in the same base pointer share one predicate and one predicated load.
;
; RUN: igc_opt --opaque-pointers --ocl --platformdg2 --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; The base (%x) dominates the merge: it is used as is by the shared load.
;
; CHECK-LABEL: define spir_kernel void @dominating_base(
; CHECK:     merge:
; CHECK:       [[ISX:%.*]] = phi i1 [ true, %bb1 ], [ true, %bb2 ], [ false, %bb3 ]
; CHECK:       [[ISY:%.*]] = phi i1 [ false, %bb1 ], [ false, %bb2 ], [ true, %bb3 ]
; CHECK-NOT:   phi
; CHECK:     use:
; CHECK:       [[VX:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %x, i64 4, i1 [[ISX]], float poison)
; CHECK:       [[VY:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %y, i64 4, i1 [[ISY]], float poison)
; CHECK-NOT:   PredicatedLoad
; CHECK:       [[V:%.*]] = select i1 [[ISX]], float [[VX]], float [[VY]]
; CHECK:       store float [[V]], ptr addrspace(1) %out, align 4
define spir_kernel void @dominating_base(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %sel, i1 %c) {
entry:
  %x = alloca [64 x float], align 4
  %y = addrspacecast ptr addrspace(1) %node to ptr
  switch i32 %sel, label %bb3 [ i32 0, label %bb1
                                i32 1, label %bb2 ]
bb1:
  br label %merge
bb2:
  br label %merge
bb3:
  br label %merge
merge:
  %p = phi ptr [ %x, %bb1 ], [ %x, %bb2 ], [ %y, %bb3 ]
  br i1 %c, label %use, label %exit
use:
  %v = load float, ptr %p, align 4
  store float %v, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

; The base (%g) does not dominate the merge (bb3 bypasses %A), so it is rebuilt at the load. Its index is carried by
; one value-level PHI that holds %idx on every edge of the group (bb1 and bb2), not 0 on one of them.
;
; CHECK-LABEL: define spir_kernel void @non_dominating_base(
; CHECK:     merge:
; CHECK:       [[ISG:%.*]] = phi i1 [ true, %bb1 ], [ true, %bb2 ], [ false, %bb3 ]
; CHECK:       [[ISY:%.*]] = phi i1 [ false, %bb1 ], [ false, %bb2 ], [ true, %bb3 ]
; CHECK:       [[IDX:%.*]] = phi i32 [ %idx, %bb1 ], [ %idx, %bb2 ], [ 0, %bb3 ]
; CHECK-NOT:   phi
; CHECK:     use:
; CHECK:       [[G:%.*]] = getelementptr inbounds float, ptr %x, i32 [[IDX]]
; CHECK:       [[VG:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[G]], i64 4, i1 [[ISG]], float poison)
; CHECK:       [[VY:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %y, i64 4, i1 [[ISY]], float poison)
; CHECK-NOT:   PredicatedLoad
; CHECK:       [[V:%.*]] = select i1 [[ISG]], float [[VG]], float [[VY]]
; CHECK:       store float [[V]], ptr addrspace(1) %out, align 4
define spir_kernel void @non_dominating_base(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %i, i1 %c0, i1 %c1, i1 %c) {
entry:
  %x = alloca [64 x float], align 4
  %y = addrspacecast ptr addrspace(1) %node to ptr
  br i1 %c0, label %A, label %bb3
A:
  %idx = add i32 %i, 1
  %g = getelementptr inbounds float, ptr %x, i32 %idx
  br i1 %c1, label %bb1, label %bb2
bb1:
  br label %merge
bb2:
  br label %merge
bb3:
  br label %merge
merge:
  %p = phi ptr [ %g, %bb1 ], [ %g, %bb2 ], [ %y, %bb3 ]
  br i1 %c, label %use, label %exit
use:
  %v = load float, ptr %p, align 4
  store float %v, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

!igc.functions = !{!0, !3}
!0 = !{ptr @dominating_base, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @non_dominating_base, !1}
