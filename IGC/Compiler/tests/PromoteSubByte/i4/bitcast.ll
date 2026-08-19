;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK-LABEL: define spir_func void @test(ptr addrspace(3) %ptr, ptr addrspace(3) %out)
; CHECK-NEXT: %1 = load <2 x i8>, ptr addrspace(3) %ptr, align 2
; CHECK-NEXT: %2 = bitcast <2 x i8> %1 to <2 x i8>
; CHECK-NEXT: store <2 x i8> %2, ptr addrspace(3) %out, align 2
; CHECK-NEXT: ret void
define spir_func void @test(ptr addrspace(3) %ptr, ptr addrspace(3) %out) {
  %1 = load <4 x i4>, ptr addrspace(3) %ptr
  %2 = bitcast <4 x i4> %1 to <2 x i8>
  store <2 x i8> %2, ptr addrspace(3) %out
  ret void
}
