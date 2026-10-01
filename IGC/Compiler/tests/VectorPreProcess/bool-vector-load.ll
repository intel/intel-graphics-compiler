;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-vectorpreprocess -S %s -o - | FileCheck %s

; A <N x i1> load cannot be split into i1 elements, so it is loaded as bytes.
; Legalization lowers the extracts, see Legalization/bool-vector-load.ll.

; CHECK-LABEL: @test_v64i1_load
; CHECK: [[LD:%.*]] = load <8 x i8>, ptr addrspace(3) %p, align 16
; CHECK: [[BC:%.*]] = bitcast <8 x i8> [[LD]] to <64 x i1>
; CHECK: extractelement <64 x i1> [[BC]], i64 9
define i1 @test_v64i1_load(ptr addrspace(3) %p) {
  %v = load <64 x i1>, ptr addrspace(3) %p, align 16
  %e = extractelement <64 x i1> %v, i64 9
  ret i1 %e
}
