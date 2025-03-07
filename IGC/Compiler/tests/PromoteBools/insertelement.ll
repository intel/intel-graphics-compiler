;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK: [[V0:%.*]] = insertelement <4 x i8> undef, i8 0, i32 0
; CHECK-NEXT: [[V1:%.*]] = insertelement <4 x i8> [[V0]], i8 1, i32 1
; CHECK-NEXT: [[V2:%.*]] = insertelement <4 x i8> [[V1]], i8 0, i32 2
; CHECK-NEXT: [[V3:%.*]] = insertelement <4 x i8> [[V2]], i8 1, i32 3
; CHECK-NEXT: store <4 x i8> [[V3]], <4 x i8> addrspace(3)* %ptr

define spir_func void @test(<4 x i1> addrspace(3)* %ptr) {
  %1 = insertelement <4 x i1> undef, i1 0, i32 0
  %2 = insertelement <4 x i1> %1, i1 1, i32 1
  %3 = insertelement <4 x i1> %2, i1 0, i32 2
  %4 = insertelement <4 x i1> %3, i1 1, i32 3
  store <4 x i1> %4, <4 x i1> addrspace(3)* %ptr
  ret void
}
