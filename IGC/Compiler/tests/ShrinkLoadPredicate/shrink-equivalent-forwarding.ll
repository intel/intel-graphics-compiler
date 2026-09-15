;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s --check-prefixes=CHECK

; The two bitcasts forward the same loaded value, so %inner cannot affect the
; result and only %outer observes the load.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define <2 x half> @equivalent_forwarding(
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %outer
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %first = bitcast i32 %a to <2 x half>
; CHECK-NEXT:   %second = bitcast i32 %a to <2 x half>
; CHECK-NEXT:   %middle = select i1 %inner, <2 x half> %first, <2 x half> %second
; CHECK-NEXT:   %r = select i1 %outer, <2 x half> %middle, <2 x half> zeroinitializer
; CHECK-NEXT:    ret <2 x half> %r

define <2 x half> @equivalent_forwarding(ptr addrspace(1) %p, i1 noundef %q, i1 noundef %inner, i1 noundef %outer) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %first = bitcast i32 %a to <2 x half>
  %second = bitcast i32 %a to <2 x half>
  %middle = select i1 %inner, <2 x half> %first, <2 x half> %second
  %r = select i1 %outer, <2 x half> %middle, <2 x half> zeroinitializer
  ret <2 x half> %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
