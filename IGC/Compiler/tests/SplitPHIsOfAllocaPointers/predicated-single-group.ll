;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
;
; Verify that when every incoming of the pointer PHI has the same base (here behind different casts), all edges
; form one group: the merged address is always that base, so the load reads it directly and no predication is added.
;
; RUN: igc_opt --opaque-pointers --ocl --platformdg2 --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; Same alloca on both edges: plain load from the alloca.
;
; CHECK-LABEL: define spir_kernel void @same_alloca(
; CHECK-NOT:   isSide
; CHECK-NOT:   phi
; CHECK:     use:
; CHECK-NEXT:  [[V:%.*]] = load float, ptr %x, align 4
; CHECK-NEXT:  store float [[V]], ptr addrspace(1) %out, align 4
; CHECK-NOT:   PredicatedLoad
; CHECK:       ret void
define spir_kernel void @same_alloca(ptr addrspace(1) %out, i32 %sel, i1 %c) {
entry:
  %x = alloca [64 x float], align 4
  switch i32 %sel, label %bb2 [ i32 0, label %bb1 ]
bb1:
  %g1 = addrspacecast ptr %x to ptr addrspace(4)
  br label %merge
bb2:
  %g2 = addrspacecast ptr %x to ptr addrspace(4)
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %g1, %bb1 ], [ %g2, %bb2 ]
  br i1 %c, label %use, label %exit
use:
  %v = load float, ptr addrspace(4) %p, align 4
  store float %v, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

; Same GEP on both edges: plain load from the GEP, keeping the original load's metadata.
;
; CHECK-LABEL: define spir_kernel void @same_gep(
; CHECK-NOT:   isSide
; CHECK-NOT:   phi
; CHECK:     use:
; CHECK-NEXT:  [[V:%.*]] = load float, ptr %g, align 4, !lsc.cache.ctrl [[$CC:![0-9]+]]
; CHECK-NEXT:  store float [[V]], ptr addrspace(1) %out, align 4
; CHECK-NOT:   PredicatedLoad
; CHECK:       ret void
define spir_kernel void @same_gep(ptr addrspace(1) %out, i32 %i, i1 %c0, i1 %c1, i1 %c) {
entry:
  %x = alloca [64 x float], align 4
  br i1 %c0, label %A, label %exit
A:
  %idx = add i32 %i, 1
  %g = getelementptr inbounds float, ptr %x, i32 %idx
  br i1 %c1, label %bb1, label %bb2
bb1:
  %g1 = addrspacecast ptr %g to ptr addrspace(4)
  br label %merge
bb2:
  %g2 = addrspacecast ptr %g to ptr addrspace(4)
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %g1, %bb1 ], [ %g2, %bb2 ]
  br i1 %c, label %use, label %exit
use:
  %v = load float, ptr addrspace(4) %p, align 4, !lsc.cache.ctrl !10
  store float %v, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

; An existing predicated load keeps its own predicate and merge value, adds no "and i1 true", and is rebuilt with the
; declaration matching the (private) base pointer.
;
; CHECK-LABEL: define spir_kernel void @same_gep_predicated_load(
; CHECK-NOT:   isSide
; CHECK-NOT:   phi
; CHECK:     use:
; CHECK-NEXT:  [[V:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %g, i64 4, i1 %c1, float 2.000000e+00)
; CHECK-NEXT:  store float [[V]], ptr addrspace(1) %out, align 4
; CHECK:       ret void
define spir_kernel void @same_gep_predicated_load(ptr addrspace(1) %out, i32 %i, i1 %c0, i1 %c1, i1 %c) {
entry:
  %x = alloca [64 x float], align 4
  br i1 %c0, label %A, label %exit
A:
  %idx = add i32 %i, 1
  %g = getelementptr inbounds float, ptr %x, i32 %idx
  br i1 %c1, label %bb1, label %bb2
bb1:
  %g1 = addrspacecast ptr %g to ptr addrspace(4)
  br label %merge
bb2:
  %g2 = addrspacecast ptr %g to ptr addrspace(4)
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %g1, %bb1 ], [ %g2, %bb2 ]
  br i1 %c, label %use, label %exit
use:
  %v = call float @llvm.genx.GenISA.PredicatedLoad.f32.p4.f32(ptr addrspace(4) %p, i64 4, i1 %c1, float 2.0)
  store float %v, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

; CHECK: [[$CC]] = !{i32 5}

declare float @llvm.genx.GenISA.PredicatedLoad.f32.p4.f32(ptr addrspace(4), i64, i1, float)

!igc.functions = !{!0, !3, !4}
!0 = !{ptr @same_alloca, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @same_gep, !1}
!4 = !{ptr @same_gep_predicated_load, !1}
!10 = !{i32 5}
