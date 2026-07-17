;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
;
; DG2 is below Xe3 (PTL), so bitcasted load narrowing is off by default. It only
; happens when the EnableBitcastedLoadNarrowing regkey is explicitly enabled.
;
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-vectorpreprocess -S %s -o - | FileCheck %s --check-prefixes=CHECK,OFF
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-vectorpreprocess -regkey EnableBitcastedLoadNarrowing=1 -S %s -o - | FileCheck %s --check-prefixes=CHECK,ON

target triple = "igil_32_GEN12"

; The <4 x i32> load is bitcast to <4 x float> and only elements 0..2 are used,
; so when narrowing is enabled the load is narrowed to a <3 x i32> load with
; per-element bitcasts. When disabled the load and bitcast are left intact.
;
; CHECK-LABEL: @bitcasted_vec_load
; OFF: call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32
; OFF: bitcast <4 x i32> %{{.*}} to <4 x float>
; ON: call <3 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v3i32
; ON-NOT: call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32
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
