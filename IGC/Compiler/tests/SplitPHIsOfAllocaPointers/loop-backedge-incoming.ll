;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
;
; Verify that PHI split handles loop backedges correctly when the incoming pointer is from the previous iteration.
;
; RUN: igc_opt --opaque-pointers --ocl --platformdg2 --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; CHECK-LABEL: define internal void @callee(
; CHECK:     hdr:
; CHECK:       [[ISENTRY:%.*]] = phi i1 [ true, %entry ], [ false, %body ]
; CHECK:       [[ISBODY:%.*]] = phi i1 [ false, %entry ], [ true, %body ]
; CHECK:       [[IDX:%.*]] = phi i32 [ 0, %entry ], [ %it.n, %body ]
; CHECK-NOT:   phi ptr
; CHECK:     body:
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %e0, i64 4, i1 [[ISENTRY]], float poison)
; CHECK:       [[PREV:%.*]] = getelementptr inbounds float, ptr %a, i32 [[IDX]]
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[PREV]], i64 4, i1 [[ISBODY]], float poison)
define spir_kernel void @kernel(ptr addrspace(1) %d, i32 %i) {
  %a = alloca [64 x float], align 4
  call void @callee(ptr %a, ptr addrspace(1) %d, i32 %i)
  ret void
}

define internal void @callee(ptr %a, ptr addrspace(1) %d, i32 %i) {
entry:
  %e0 = getelementptr inbounds float, ptr %a, i32 %i
  br label %hdr
hdr:
  %it = phi i32 [ 0, %entry ], [ %it.n, %body ]
  %acc = phi float [ 0.0, %entry ], [ %acc.n, %body ]
  %p = phi ptr [ %e0, %entry ], [ %e1, %body ]
  br label %body
body:
  %it.n = add i32 %it, 1
  %e1 = getelementptr inbounds float, ptr %a, i32 %it.n
  %v = load float, ptr %p, align 4
  %acc.n = fadd float %acc, %v
  %lc = icmp ult i32 %it.n, 3
  br i1 %lc, label %hdr, label %out
out:
  store float %acc.n, ptr addrspace(1) %d, align 4
  ret void
}

!igc.functions = !{!0, !2}
!0 = !{ptr @kernel, !1}
!1 = !{!3}
!2 = !{ptr @callee, !4}
!3 = !{!"function_type", i32 0}
!4 = !{!5}
!5 = !{!"function_type", i32 1}
