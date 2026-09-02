;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -igc-wi-analysis -print-wia-check --disable-output --regkey=PrintToConsole=1 2>&1 | FileCheck %s --check-prefixes=CHECK,ENABLED
; RUN: igc_opt %s --opaque-pointers -igc-wi-analysis -print-wia-check --disable-output --regkey=PrintToConsole=1,DisableUniformAnalysis=1 2>&1 | FileCheck %s --check-prefixes=CHECK,DISABLED

; CHECK-LABEL: define spir_kernel void @kernel()
; ENABLED: BB:0 entry {{.*}}[ uniform_global ]
; DISABLED: BB:0 entry {{.*}}[ random  ]
; CHECK: BB:1 divergent {{.*}}[ random  ]
; ENABLED: BB:2 exit {{.*}}[ uniform_global ]
; DISABLED: BB:2 exit {{.*}}[ random  ]
define spir_kernel void @kernel() {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %half = icmp ult i16 %lane, 8
  br i1 %half, label %divergent, label %exit

divergent:
  call void @callee()
  br label %exit

exit:
  ret void
}

; CHECK-LABEL: define internal void @callee()
; CHECK: BB:0 entry {{.*}}[ random  ]
define internal void @callee() {
entry:
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !1}
!0 = !{ptr @kernel, !2}
!1 = !{ptr @callee, !3}
!2 = !{!{!"function_type", i32 0}}
!3 = !{!{!"function_type", i32 2}}
