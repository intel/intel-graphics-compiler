;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1,ForceOCLSIMDWidth=16 -platformPtl < %s 2>&1  | FileCheck %s

define spir_kernel void @foo_3() {
entry:
  %ptr = alloca i32, align 4
  %cast_slm = addrspacecast ptr %ptr to ptr addrspace(3)
  %val_2 = load i32, ptr addrspace(3) %cast_slm, align 4
  ret void
}

define spir_kernel void @foo_1() {
entry:
  %ptr = alloca i32, align 4
  %cast_global = addrspacecast ptr %ptr to ptr addrspace(1)
  %val_1 = load i32, ptr addrspace(1) %cast_global, align 4
  ret void
}

define spir_kernel void @foo_0() {
entry:
  %ptr = alloca i32, align 4
  %val = load i32, ptr %ptr, align 4
  ret void
}

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}

; CHECK: SIMD: 16, external pressure: 0
; CHECK: block: entry function: foo_3
; CHECK: IN:     [       0       ]
; CHECK: OUT:    [       0       ]
; CHECK: N: 64 (1)            %ptr = alloca i32, align 4
; CHECK: N: 64 (1)            %cast_slm = addrspacecast ptr %ptr to ptr addrspace(3)
; CHECK: N: 0  (0)            %val_2 = load i32, ptr addrspace(3) %cast_slm, align 4
; CHECK: N: 0  (0)            ret void

; CHECK: SIMD: 16, external pressure: 0
; CHECK: block: entry function: foo_1
; CHECK: IN:     [       0       ]
; CHECK: OUT:    [       0       ]
; CHECK: N: 64  (1)           %ptr = alloca i32, align 4
; CHECK: N: 128 (2)           %cast_global = addrspacecast ptr %ptr to ptr addrspace(1)
; CHECK: N: 0   (0)           %val_1 = load i32, ptr addrspace(1) %cast_global, align 4
; CHECK: N: 0   (0)           ret void

; CHECK: SIMD: 16, external pressure: 0
; CHECK: block: entry function: foo_0
; CHECK: IN:     [       0       ]
; CHECK: OUT:    [       0       ]
; CHECK: N: 64 (1)            %ptr = alloca i32, align 4
; CHECK: N: 0  (0)            %val = load i32, ptr %ptr, align 4
; CHECK: N: 0  (0)            ret void
