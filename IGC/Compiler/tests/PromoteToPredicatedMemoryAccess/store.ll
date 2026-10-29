;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --enable-debugify --igc-promote-to-predicated-memory-access --platformbmg -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --enable-debugify --igc-promote-to-predicated-memory-access --platformbmg -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; basic test
; CHECK-LABEL: @test1(
define void @test1(<64 x i32> addrspace(1)* %dst, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br label %st
  br i1 %pred, label %st, label %exit

st:
; CHECK-TYPED: call void @llvm.genx.GenISA.PredicatedStore.p1v64i32.v64i32(<64 x i32> addrspace(1)* %dst, <64 x i32> %data, i64 4, i1 %pred)
; CHECK-OPAQUE: call void @llvm.genx.GenISA.PredicatedStore.p1.v64i32(ptr addrspace(1) %dst, <64 x i32> %data, i64 4, i1 %pred)
  store <64 x i32> %data, <64 x i32> addrspace(1)* %dst, align 4
  br label %exit

exit:
  ret void
}

; inverse test
; CHECK-LABEL: @test2(
define void @test2(<64 x i32> addrspace(1)* %dst, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br label %st
  br i1 %pred, label %exit, label %st

st:
; CHECK: [[NOT:%.*]] = xor i1 %pred, true
; CHECK-TYPED: call void @llvm.genx.GenISA.PredicatedStore.p1v64i32.v64i32(<64 x i32> addrspace(1)* %dst, <64 x i32> %data, i64 4, i1 [[NOT]])
; CHECK-OPAQUE: call void @llvm.genx.GenISA.PredicatedStore.p1.v64i32(ptr addrspace(1) %dst, <64 x i32> %data, i64 4, i1 [[NOT]])
  store <64 x i32> %data, <64 x i32> addrspace(1)* %dst, align 4
  br label %exit

exit:
  ret void
}

; early exit, not simple
; CHECK-LABEL: @test3(
define void @test3(<64 x i32> addrspace(1)* %dst, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK-TYPED: store volatile <64 x i32> %data, <64 x i32> addrspace(1)* %dst, align 4
; CHECK-OPAQUE: store volatile <64 x i32> %data, ptr addrspace(1) %dst, align 4
  store volatile <64 x i32> %data, <64 x i32> addrspace(1)* %dst, align 4
  br label %exit

exit:
  ret void
}

; illegal int type, exit
; CHECK-LABEL: @test4(
define void @test4(<64 x i33> addrspace(1)* %dst, i1 %pred, <64 x i33> %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK-TYPED: store <64 x i33> %data, <64 x i33> addrspace(1)* %dst, align 4
; CHECK-OPAQUE: store <64 x i33> %data, ptr addrspace(1) %dst, align 4
  store <64 x i33> %data, <64 x i33> addrspace(1)* %dst, align 4
  br label %exit

exit:
  ret void
}

; basic test with phi in store block
; CHECK-LABEL: @test5(
; CHECK: br label %st
; CHECK-TYPED: call void @llvm.genx.GenISA.PredicatedStore.p1v64i32.v64i32(<64 x i32> addrspace(1)* %dst, <64 x i32> %phi, i64 4, i1 %pred)
; CHECK-OPAQUE: call void @llvm.genx.GenISA.PredicatedStore.p1.v64i32(ptr addrspace(1) %dst, <64 x i32> %phi, i64 4, i1 %pred)
;
define void @test5(<64 x i32> addrspace(1)* %dst, i1 %pred, <64 x i32> %data) {
entry:
  br i1 %pred, label %st, label %exit
st:
  %phi = phi <64 x i32> [ %data, %entry ]
  store <64 x i32> %phi, <64 x i32> addrspace(1)* %dst, align 4
  br label %exit
exit:
  ret void
}
