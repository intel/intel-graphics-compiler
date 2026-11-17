;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --ocl --platformPtl --igc-split-selects-of-alloca-pointers \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=0 \
; RUN:   -verify -S %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --ocl --platformPtl --igc-split-selects-of-alloca-pointers \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=0 \
; RUN:   -verify -S %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}
;
; The alloca-rooted select operand must keep its address in the alloca's address space when the post-select cast chain is replayed on it.

; CHECK-LABEL: define spir_kernel void @clamp_global(
; CHECK-TYPED:   [[EXT:%.*]] = addrspacecast float addrspace(4)* %ext to float addrspace(1)*
; CHECK-OPAQUE:  [[EXT:%.*]] = addrspacecast ptr addrspace(4) %ext to ptr addrspace(1)
; CHECK-TYPED:   [[L0:%.*]] = load float, float* %g, align 4
; CHECK-OPAQUE:  [[L0:%.*]] = load float, ptr %g, align 4
; CHECK-TYPED:   [[L1:%.*]] = load float, float addrspace(1)* [[EXT]], align 4
; CHECK-OPAQUE:  [[L1:%.*]] = load float, ptr addrspace(1) [[EXT]], align 4
; CHECK:         select i1 %c, float [[L0]], float [[L1]]
; CHECK-NOT:     Predicated
; CHECK-NOT:     select i1 %c, {{.*}}*
; CHECK-NOT:     select i1 %c, ptr
; CHECK:         ret void

define spir_kernel void @clamp_global(float addrspace(4)* dereferenceable(4) align 4 %ext, float addrspace(1)* %out, i1 %c, i64 %i) #0 {
entry:
  %arr = alloca [64 x float], align 4
  %g = getelementptr inbounds [64 x float], [64 x float]* %arr, i64 0, i64 %i
  %ga = addrspacecast float* %g to float addrspace(4)*
  %p = select i1 %c, float addrspace(4)* %ga, float addrspace(4)* %ext
  %pg = addrspacecast float addrspace(4)* %p to float addrspace(1)*
  %v = load float, float addrspace(1)* %pg, align 4
  store float %v, float addrspace(1)* %out, align 4
  ret void
}

; Empty post-select chain: stripping the alloca operand's bitcast loses the
; pointee type under typed pointers, so it must be restored before the load.
;
; CHECK-LABEL: define spir_kernel void @empty_chain(
; CHECK-TYPED:   [[A:%.*]] = bitcast [64 x float]* %arr to float*
; CHECK-TYPED:   [[L0:%.*]] = load float, float* [[A]], align 4
; CHECK-OPAQUE:  [[L0:%.*]] = load float, ptr %arr, align 4
; CHECK-TYPED:   [[L1:%.*]] = load float, float* %other, align 4
; CHECK-OPAQUE:  [[L1:%.*]] = load float, ptr %other, align 4
; CHECK:         select i1 %c, float [[L0]], float [[L1]]

define spir_kernel void @empty_chain(float* dereferenceable(4) align 4 %other, float addrspace(1)* %out, i1 %c) #0 {
entry:
  %arr = alloca [64 x float], align 4
  %a = bitcast [64 x float]* %arr to float*
  %p = select i1 %c, float* %a, float* %other
  %v = load float, float* %p, align 4
  store float %v, float addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { nofree nosync }

!igc.functions = !{!0, !3}
!0 = !{void (float addrspace(4)*, float addrspace(1)*, i1, i64)* @clamp_global, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (float*, float addrspace(1)*, i1)* @empty_chain, !1}
!IGCMetadata = !{!4}
!4 = !{!"ModuleMD", !5}
!5 = !{!"compOpt", !6}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
