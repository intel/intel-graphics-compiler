;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
;
; Test ForceStackCallForLargeKernel: with FunctionControl default, functions of a kernel whose
; estimated size exceeds the large-kernel threshold default to stack calls.
;
; Large kernel threshold lowered and flag on -> helper becomes a stack call.
; RUN: igc_opt --igc-process-func-attributes -S \
; RUN:   -regkey KernelTotalSizeThreshold=1 -regkey LargeKernelThresholdMultiplier=1 \
; RUN:   -regkey ForceStackCallForLargeKernel=1 \
; RUN:   < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LARGE
;
; Large kernel but flag explicitly disabled -> helper stays inlinable, no stack call.
; RUN: igc_opt --igc-process-func-attributes -S \
; RUN:   -regkey KernelTotalSizeThreshold=1 -regkey LargeKernelThresholdMultiplier=1 \
; RUN:   -regkey ForceStackCallForLargeKernel=0 \
; RUN:   < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-OFF
;
; Flag on but kernel below threshold (default size) -> not a large kernel, no stack call.
; RUN: igc_opt --igc-process-func-attributes -S \
; RUN:   -regkey KernelTotalSizeThreshold=50000 -regkey LargeKernelThresholdMultiplier=12 \
; RUN:   -regkey ForceStackCallForLargeKernel=1 \
; RUN:   < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-SMALL
; ------------------------------------------------

; CHECK-LARGE: define internal spir_func i32 @helper({{.*}}) #[[ATTR:[0-9]+]]
; CHECK-OFF-NOT: visaStackCall
; CHECK-SMALL-NOT: visaStackCall

define spir_func i32 @helper(i32 %x) #0 {
  %1 = add i32 %x, 144
  %2 = mul i32 %1, 13
  ret i32 %2
}

define spir_kernel void @test_kernel(i32 %a) #0 {
  %1 = call i32 @helper(i32 %a)
  ret void
}

attributes #0 = { nounwind }

; CHECK-LARGE: attributes #[[ATTR]] = { noinline nounwind "visaStackCall" }

!igc.functions = !{!1}
!1 = !{void (i32)* @test_kernel, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
