;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Calls that do not access memory and have no side effects (llvm.smax/smin)
; do not stop load and store combining. Calls that may access memory or may
; not return still do.

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers %s -S -igc-ldstcombine -regkey=EnableLdStCombine=5 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

; CHECK-LABEL: define spir_kernel void @store_clamp
; CHECK-NOT: store
; CHECK: [[V:%.*]] = call i32 @llvm.genx.GenISA.bitcastfromstruct.i32
; CHECK-NEXT: store i32 [[V]], ptr addrspace(1) %dst, align 4
; CHECK-NEXT: ret void
define spir_kernel void @store_clamp(ptr addrspace(1) %dst, i32 %r, i32 %g, i32 %b) {
entry:
  %r.max = call i32 @llvm.smax.i32(i32 %r, i32 0)
  %r.min = call i32 @llvm.smin.i32(i32 %r.max, i32 255)
  %r.b = trunc i32 %r.min to i8
  store i8 %r.b, ptr addrspace(1) %dst, align 4
  %g.max = call i32 @llvm.smax.i32(i32 %g, i32 0)
  %g.min = call i32 @llvm.smin.i32(i32 %g.max, i32 255)
  %g.b = trunc i32 %g.min to i8
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 1
  store i8 %g.b, ptr addrspace(1) %p1, align 1
  %b.max = call i32 @llvm.smax.i32(i32 %b, i32 0)
  %b.min = call i32 @llvm.smin.i32(i32 %b.max, i32 255)
  %b.b = trunc i32 %b.min to i8
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 2
  store i8 %b.b, ptr addrspace(1) %p2, align 2
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 3
  store i8 -1, ptr addrspace(1) %p3, align 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @store_unknown_call
; CHECK: store i16 {{.*}}, ptr addrspace(1) %dst, align 4
; CHECK-NEXT: call void @foo()
; CHECK: store i16 {{.*}}, ptr addrspace(1) %p2, align 2
; CHECK-NEXT: ret void
define spir_kernel void @store_unknown_call(ptr addrspace(1) %dst, i8 %a, i8 %b, i8 %c, i8 %d) {
entry:
  store i8 %a, ptr addrspace(1) %dst, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 1
  store i8 %b, ptr addrspace(1) %p1, align 1
  call void @foo()
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 2
  store i8 %c, ptr addrspace(1) %p2, align 2
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 3
  store i8 %d, ptr addrspace(1) %p3, align 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @store_may_not_return
; CHECK: store i16 {{.*}}, ptr addrspace(1) %dst, align 4
; CHECK-NEXT: call i32 @bar(
; CHECK: store i16 {{.*}}, ptr addrspace(1) %p2, align 2
; CHECK-NEXT: ret void
define spir_kernel void @store_may_not_return(ptr addrspace(1) %dst, i8 %a, i8 %b, i8 %c, i32 %x) {
entry:
  store i8 %a, ptr addrspace(1) %dst, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 1
  store i8 %b, ptr addrspace(1) %p1, align 1
  %y = call i32 @bar(i32 %x)
  %y.b = trunc i32 %y to i8
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 2
  store i8 %c, ptr addrspace(1) %p2, align 2
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 3
  store i8 %y.b, ptr addrspace(1) %p3, align 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @load_clamp
; CHECK: load i32, ptr addrspace(1) %src, align 4
; CHECK-NOT: load
; CHECK: ret void
define spir_kernel void @load_clamp(ptr addrspace(1) %dst, ptr addrspace(1) %src) {
entry:
  %c0 = load i8, ptr addrspace(1) %src, align 4
  %c0.e = zext i8 %c0 to i32
  %c0.m = call i32 @llvm.smin.i32(i32 %c0.e, i32 100)
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 1
  %c1 = load i8, ptr addrspace(1) %p1, align 1
  %c1.e = zext i8 %c1 to i32
  %c1.m = call i32 @llvm.smin.i32(i32 %c1.e, i32 100)
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 2
  %c2 = load i8, ptr addrspace(1) %p2, align 2
  %c2.e = zext i8 %c2 to i32
  %c2.m = call i32 @llvm.smin.i32(i32 %c2.e, i32 100)
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 3
  %c3 = load i8, ptr addrspace(1) %p3, align 1
  %c3.e = zext i8 %c3 to i32
  %s0 = add i32 %c0.m, %c1.m
  %s1 = add i32 %c2.m, %c3.e
  %s = add i32 %s0, %s1
  store i32 %s, ptr addrspace(1) %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @load_unknown_call
; CHECK: load <2 x i8>, ptr addrspace(1) %src, align 4
; CHECK: call void @foo()
; CHECK: load <2 x i8>, ptr addrspace(1) %p2, align 2
; CHECK-NOT: load
; CHECK: ret void
define spir_kernel void @load_unknown_call(ptr addrspace(1) %dst, ptr addrspace(1) %src) {
entry:
  %c0 = load i8, ptr addrspace(1) %src, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 1
  %c1 = load i8, ptr addrspace(1) %p1, align 1
  call void @foo()
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 2
  %c2 = load i8, ptr addrspace(1) %p2, align 2
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 3
  %c3 = load i8, ptr addrspace(1) %p3, align 1
  %v0 = insertelement <4 x i8> poison, i8 %c0, i32 0
  %v1 = insertelement <4 x i8> %v0, i8 %c1, i32 1
  %v2 = insertelement <4 x i8> %v1, i8 %c2, i32 2
  %v3 = insertelement <4 x i8> %v2, i8 %c3, i32 3
  store <4 x i8> %v3, ptr addrspace(1) %dst, align 4
  ret void
}

declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.smin.i32(i32, i32)
declare void @foo()
declare i32 @bar(i32) #0

attributes #0 = { nounwind readnone }
