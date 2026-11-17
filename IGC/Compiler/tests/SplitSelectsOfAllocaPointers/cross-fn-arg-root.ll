;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
;
; Verify that SELECTs of alloca pointers are split correctly across function calls.
;
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-split-selects-of-alloca-pointers \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=1 \
; RUN:   --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-split-selects-of-alloca-pointers \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=1 \
; RUN:   --regkey EnableSOAAcrossCalls=0 -S %s | FileCheck %s --check-prefix=NOSPLIT

; CHECK-LABEL: define internal float @callee(
; CHECK: [[SV:%.*]] = load float, ptr %e, align 4
; CHECK: [[GV:%.*]] = load float, ptr addrspace(4) %gn, align 4
; CHECK: select i1 %c, float [[SV]], float [[GV]]
; CHECK-NOT: select i1 %c, ptr

; NOSPLIT-LABEL: define internal float @callee(
; NOSPLIT: select i1 %c, ptr addrspace(4)

define spir_kernel void @kernel(ptr addrspace(1) %d, i32 %i, i1 %c) {
  %a = alloca [64 x float], align 4
  %v = call float @callee(ptr %a, ptr addrspace(1) %d, i32 %i, i1 %c)
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

define internal float @callee(ptr %stack, ptr addrspace(1) %node, i32 %i, i1 %c) {
  %e = getelementptr inbounds float, ptr %stack, i32 %i
  %ge = addrspacecast ptr %e to ptr addrspace(4)
  %gn = addrspacecast ptr addrspace(1) %node to ptr addrspace(4)
  %p = select i1 %c, ptr addrspace(4) %ge, ptr addrspace(4) %gn
  %v = load float, ptr addrspace(4) %p, align 4
  ret float %v
}

!IGCMetadata = !{!6}
!igc.functions = !{!0, !2}
!0 = !{ptr @kernel, !1}
!1 = !{!4}
!2 = !{ptr @callee, !3}
!3 = !{!5}
!4 = !{!"function_type", i32 0}
!5 = !{!"function_type", i32 1}
!6 = !{!"ModuleMD", !7}
!7 = !{!"compOpt", !8}
!8 = !{!"UseScratchSpacePrivateMemory", i1 false}
