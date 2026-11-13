;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
;
; Verify that PHIs of alloca pointers are split correctly across function calls.
;
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=0 -S %s | FileCheck %s --check-prefix=NOSPLIT

; CHECK-LABEL: define internal float @callee(
; CHECK: then:
; CHECK: [[SV:%.*]] = load float, ptr {{%.*}}, align 4
; CHECK: else:
; CHECK: [[GV:%.*]] = load float, ptr addrspace(4) {{%.*}}, align 4
; CHECK: merge:
; CHECK: phi float [ [[SV]], %then ], [ [[GV]], %else ]
; CHECK-NOT: phi ptr

; NOSPLIT-LABEL: define internal float @callee(
; NOSPLIT: phi ptr addrspace(4)

define spir_kernel void @kernel(ptr addrspace(1) %d, i32 %i, i1 %c) {
  %a = alloca [64 x float], align 4
  %v = call float @callee(ptr %a, ptr addrspace(1) %d, i32 %i, i1 %c)
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

define internal float @callee(ptr %stack, ptr addrspace(1) %node, i32 %i, i1 %c) {
entry:
  br i1 %c, label %then, label %else
then:
  %e = getelementptr inbounds float, ptr %stack, i32 %i
  %ge = addrspacecast ptr %e to ptr addrspace(4)
  br label %merge
else:
  %gn = addrspacecast ptr addrspace(1) %node to ptr addrspace(4)
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %ge, %then ], [ %gn, %else ]
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
