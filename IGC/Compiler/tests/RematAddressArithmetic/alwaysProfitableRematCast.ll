;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -igc-clone-address-arithmetic --regkey=RematFlowThreshold=1 --regkey=RematRPELimit=0 --dce | FileCheck %s

define spir_kernel void @test(ptr addrspace(1) %p0, ptr addrspace(1) %p1, ptr addrspace(1) %p2, ptr addrspace(1) %p3,
                              i32 %n, i32 %v, <8 x i32> %r0) {
entry:
  %tid = extractelement <8 x i32> %r0, i64 1
  %idx = mul i32 %tid, %n
  %sext = sext i32 %idx to i64
  %g0 = getelementptr i32, ptr addrspace(1) %p0, i64 %sext
  %g1 = getelementptr i32, ptr addrspace(1) %p1, i64 %sext
  %g2 = getelementptr i32, ptr addrspace(1) %p2, i64 %sext
  %g3 = getelementptr i32, ptr addrspace(1) %p3, i64 %sext
  store i32 %v, ptr addrspace(1) %g0, align 4
  store i32 %v, ptr addrspace(1) %g1, align 4
  store i32 %v, ptr addrspace(1) %g2, align 4
  store i32 %v, ptr addrspace(1) %g3, align 4
  ret void
}

; CHECK-LABEL: entry:
; CHECK: %idx = mul i32 %tid, %n

; CHECK: [[S0:%remat[0-9]*]] = sext i32 %idx to i64
; CHECK: [[G0:%cloned_[a-zA-Z0-9_]*]] = getelementptr i32, ptr addrspace(1) %p0, i64 [[S0]]
; CHECK: store i32 %v, ptr addrspace(1) [[G0]]

; CHECK: [[S1:%remat[0-9]*]] = sext i32 %idx to i64
; CHECK: [[G1:%cloned_[a-zA-Z0-9_]*]] = getelementptr i32, ptr addrspace(1) %p1, i64 [[S1]]
; CHECK: store i32 %v, ptr addrspace(1) [[G1]]

; CHECK: [[S2:%remat[0-9]*]] = sext i32 %idx to i64
; CHECK: [[G2:%cloned_[a-zA-Z0-9_]*]] = getelementptr i32, ptr addrspace(1) %p2, i64 [[S2]]
; CHECK: store i32 %v, ptr addrspace(1) [[G2]]

; CHECK: [[S3:%remat[0-9]*]] = sext i32 %idx to i64
; CHECK: [[G3:%cloned_[a-zA-Z0-9_]*]] = getelementptr i32, ptr addrspace(1) %p3, i64 [[S3]]
; CHECK: store i32 %v, ptr addrspace(1) [[G3]]

define spir_kernel void @test_narrowing_cast(ptr addrspace(1) %p0, ptr addrspace(1) %p1, ptr addrspace(1) %p2,
                                             ptr addrspace(1) %p3, i64 %a, i64 %b, i32 %v) {
entry:
  %wide = mul i64 %a, %b
  %tr = trunc i64 %wide to i32
  %g0 = getelementptr i32, ptr addrspace(1) %p0, i32 %tr
  %g1 = getelementptr i32, ptr addrspace(1) %p1, i32 %tr
  %g2 = getelementptr i32, ptr addrspace(1) %p2, i32 %tr
  %g3 = getelementptr i32, ptr addrspace(1) %p3, i32 %tr
  store i32 %v, ptr addrspace(1) %g0, align 4
  store i32 %v, ptr addrspace(1) %g1, align 4
  store i32 %v, ptr addrspace(1) %g2, align 4
  store i32 %v, ptr addrspace(1) %g3, align 4
  ret void
}

; CHECK-LABEL: @test_narrowing_cast
; CHECK: %tr = trunc i64 %wide to i32
; CHECK-NOT: = trunc i64
; CHECK: getelementptr i32, ptr addrspace(1) %p3, i32 %tr
; CHECK: ret void


define spir_kernel void @test_ptrtoint_cast(ptr addrspace(1) %p, ptr addrspace(1) %q0, ptr addrspace(1) %q1,
                                            ptr addrspace(1) %q2, ptr addrspace(1) %q3, i32 %v) {
entry:
  %pi = ptrtoint ptr addrspace(1) %p to i32
  %g0 = getelementptr i32, ptr addrspace(1) %q0, i32 %pi
  %g1 = getelementptr i32, ptr addrspace(1) %q1, i32 %pi
  %g2 = getelementptr i32, ptr addrspace(1) %q2, i32 %pi
  %g3 = getelementptr i32, ptr addrspace(1) %q3, i32 %pi
  store i32 %v, ptr addrspace(1) %g0, align 4
  store i32 %v, ptr addrspace(1) %g1, align 4
  store i32 %v, ptr addrspace(1) %g2, align 4
  store i32 %v, ptr addrspace(1) %g3, align 4
  ret void
}

; CHECK-LABEL: @test_ptrtoint_cast
; CHECK: %pi = ptrtoint ptr addrspace(1) %p to i32
; CHECK-NOT: = ptrtoint ptr addrspace(1) %p
; CHECK: getelementptr i32, ptr addrspace(1) %q3, i32 %pi
; CHECK: ret void

!igc.functions = !{}
