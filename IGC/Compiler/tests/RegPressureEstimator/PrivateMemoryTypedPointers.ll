;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows, llvm-17-plus
; REQUIRES: regkeys

; RUN: igc_opt -typed-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=6,ForceOCLSIMDWidth=16 -platformPtl < %s 2>&1  | FileCheck %s

define spir_kernel void @foo() {
entry:
  %ptr = alloca i32, align 4
  %val = load i32, i32* %ptr, align 4
  ret void
}

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}

; CHECK: SIMD: 16, external pressure: 0
; CHECK: block: entry function: foo
; CHECK: IN:     [       0       ]
; CHECK: OUT:    [       0       ]
; CHECK: N: UP: 0 NP: 1  64 (1)           %ptr = alloca i32, align 4
; CHECK: N: UP: 0 NP: 0  0  (0)           %val = load i32, i32* %ptr, align 4
; CHECK: N: UP: 0 NP: 0  0  (0)           ret void
