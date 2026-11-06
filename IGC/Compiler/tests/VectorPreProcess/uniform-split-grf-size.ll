;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -ocl -platformdg2 -igc-vectorpreprocess -S %s | FileCheck %s --check-prefix=GRF32
; RUN: igc_opt --opaque-pointers -ocl -platformbmg -igc-vectorpreprocess -S %s | FileCheck %s --check-prefix=GRF64

; A uniform access is split into transposed LSC messages of at most 8 GRFs:
; 256 bytes with 32-byte GRFs, 512 bytes with 64-byte GRFs.

; GRF32-LABEL: @test(
; GRF32-COUNT-2: load <64 x i32>, ptr addrspace(1) {{%.*}}, align 8
; GRF32-NOT:     load
; GRF32-COUNT-2: store <64 x i32> {{%.*}}, ptr addrspace(1) {{%.*}}, align 8
; GRF32-NOT:     store

; GRF64-LABEL: @test(
; GRF64:         load <128 x i32>, ptr addrspace(1) %src, align 8
; GRF64:         store <128 x i32> {{%.*}}, ptr addrspace(1) %dst, align 8

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
  %v = load <128 x i32>, ptr addrspace(1) %src, align 8
  store <128 x i32> %v, ptr addrspace(1) %dst, align 8
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
