;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
;
; The DisableBitcastedLoadNarrowing module flag, set from the
; WaDisableBitcastedLoadNarrowing AIL, opts a workload out of bitcasted load
; narrowing even on platforms where it is enabled by default (Xe3+).
;
; RUN: igc_opt --opaque-pointers -platformPtl -igc-vectorpreprocess -S %s -o - | FileCheck %s

target triple = "igil_32_GEN12"

; The <4 x i32> load is bitcast to <4 x float> and only elements 0..2 are used.
; With the AIL applied the original load and bitcast are left intact.
;
; CHECK-LABEL: @bitcasted_vec_load
; CHECK: call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32
; CHECK: bitcast <4 x i32> %{{.*}} to <4 x float>
; CHECK-NOT: call <3 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v3i32
define void @bitcasted_vec_load(ptr addrspace(2490368) %buf, i32 %off) {
  %v = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32(ptr addrspace(2490368) %buf, i32 %off, i32 4, i1 false)
  %bc = bitcast <4 x i32> %v to <4 x float>
  %e0 = extractelement <4 x float> %bc, i32 0
  %e1 = extractelement <4 x float> %bc, i32 1
  %e2 = extractelement <4 x float> %bc, i32 2
  call void @use_f(float %e0)
  call void @use_f(float %e1)
  call void @use_f(float %e2)
  ret void
}

declare void @use_f(float)
declare <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32(ptr addrspace(2490368), i32, i32, i1)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"DisableBitcastedLoadNarrowing", i1 true}
