;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
;
; On Panther Lake (Xe3) bitcasted load narrowing is enabled by default, with no
; regkey needed. An explicitly set EnableBitcastedLoadNarrowing=0 regkey still
; takes precedence and disables it.
;
; RUN: igc_opt --opaque-pointers -platformPtl -igc-vectorpreprocess -S %s -o - | FileCheck %s --check-prefixes=CHECK,ON
; RUN: igc_opt --opaque-pointers -platformPtl -igc-vectorpreprocess -regkey EnableBitcastedLoadNarrowing=0 -S %s -o - | FileCheck %s --check-prefixes=CHECK,OFF

target triple = "igil_32_GEN12"

; The <4 x i32> load is bitcast to <4 x float> and only elements 0..2 are used.
; Enabled (PTL default): narrowed to a <3 x i32> load. Disabled (regkey=0): the
; original load and bitcast are left intact.
;
; CHECK-LABEL: @bitcasted_vec_load
; ON: call <3 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v3i32
; ON-NOT: call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32
; OFF: call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32
; OFF: bitcast <4 x i32> %{{.*}} to <4 x float>
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
