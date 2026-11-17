;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
;
; Verify that on the scratch path the loads a select split duplicates are speculative only if both select operands
; are known to be dereferenceable.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-split-selects-of-alloca-pointers \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=0 \
; RUN:   --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s --check-prefixes=CHECK,LSC
; RUN: igc_opt --opaque-pointers --ocl --platformdg1 --igc-split-selects-of-alloca-pointers \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=0 \
; RUN:   --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s --check-prefixes=CHECK,NOLSC

; Both operands are constant in-bounds GEPs of allocas: the loads can be speculative on both platforms.
; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK:         [[A:%.*]] = load float, ptr %a1, align 4
; CHECK:         [[B:%.*]] = load float, ptr %b2, align 4
; CHECK:         select i1 %c, float [[A]], float [[B]]
; CHECK-NOT:     Predicated

; %node is a global null: the program loads only from the argument (%c is true), so the load through %node must not
; be executed.
; CHECK-LABEL: define internal float @callee(
; LSC:           [[NOTC:%.*]] = xor i1 %c, true
; LSC:           [[SV:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %stack, i64 4, i1 %c, float poison)
; LSC:           [[GV:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p4.f32(ptr addrspace(4) %gn, i64 4, i1 [[NOTC]], float poison)
; LSC:           select i1 %c, float [[SV]], float [[GV]]
; NOLSC:         %p = select i1 %c, ptr addrspace(4) %ge, ptr addrspace(4) %gn
; NOLSC:         load float, ptr addrspace(4) %p, align 4
; NOLSC-NOT:     load float, ptr addrspace(4) %gn

define spir_kernel void @kernel(ptr addrspace(1) %out, i1 %c) {
  %a = alloca [64 x float], align 4
  %b = alloca [64 x float], align 4
  store float 1.0, ptr %a, align 4
  %v = call float @callee(ptr %a, ptr addrspace(1) null, i1 %c)
  store float %v, ptr addrspace(1) %out, align 4
  %a1 = getelementptr inbounds [64 x float], ptr %a, i64 0, i64 1
  %b2 = getelementptr inbounds [64 x float], ptr %b, i64 0, i64 2
  %p = select i1 %c, ptr %a1, ptr %b2
  %w = load float, ptr %p, align 4
  store float %w, ptr addrspace(1) %out, align 4
  ret void
}

define internal float @callee(ptr %stack, ptr addrspace(1) %node, i1 %c) {
  %ge = addrspacecast ptr %stack to ptr addrspace(4)
  %gn = addrspacecast ptr addrspace(1) %node to ptr addrspace(4)
  %p = select i1 %c, ptr addrspace(4) %ge, ptr addrspace(4) %gn
  %v = load float, ptr addrspace(4) %p, align 4
  ret float %v
}

!igc.functions = !{!0, !2}
!0 = !{ptr @kernel, !1}
!1 = !{!4}
!2 = !{ptr @callee, !3}
!3 = !{!5}
!4 = !{!"function_type", i32 0}
!5 = !{!"function_type", i32 1}
