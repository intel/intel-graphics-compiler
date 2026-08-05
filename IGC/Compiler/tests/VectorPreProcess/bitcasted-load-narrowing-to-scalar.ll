;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
;
; Bitcasted load narrowing is forced on (EnableBitcastedLoadNarrowing=1) for both
; runs so the bitcast pattern is recognized; the EnableBitcastedLoadNarrowingToScalar
; regkey then decides whether a single-use bitcasted load collapses all the way to
; a scalar ldraw or stops at the <2 x i32> workaround.
;
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-vectorpreprocess -regkey EnableBitcastedLoadNarrowing=1 -regkey EnableBitcastedLoadNarrowingToScalar=0 -S %s -o - | FileCheck %s --check-prefixes=CHECK,WA
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-vectorpreprocess -regkey EnableBitcastedLoadNarrowing=1 -regkey EnableBitcastedLoadNarrowingToScalar=1 -S %s -o - | FileCheck %s --check-prefixes=CHECK,SCALAR

target triple = "igil_32_GEN12"

; A bitcasted <4 x i32> -> <4 x float> load with a single use of element 0.
; With narrowing-to-scalar disabled, a workaround keeps a <2 x i32> vector load;
; with it enabled, the load becomes a scalar ldraw followed by a bitcast to float.
;
; CHECK-LABEL: @bitcasted_scalar_load
; WA: call <2 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v2i32
; WA-NOT: @llvm.genx.GenISA.ldraw.indexed
; SCALAR: call {{.*}} @llvm.genx.GenISA.ldraw.indexed
; SCALAR: bitcast i32 %{{.*}} to float
define void @bitcasted_scalar_load(ptr addrspace(2490368) %buf, i32 %off) {
  %v = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32(ptr addrspace(2490368) %buf, i32 %off, i32 4, i1 false)
  %bc = bitcast <4 x i32> %v to <4 x float>
  %e0 = extractelement <4 x float> %bc, i32 0
  call void @use_f(float %e0)
  ret void
}

declare void @use_f(float)
declare <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32(ptr addrspace(2490368), i32, i32, i1)
