;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops - ptrtoaddr
;
; Similarly to addrspacecast, ptrtoaddr is kept as is (clearing the address
; space tag of a generic pointer is done in EmitPass), only its result is split
; into Lo and Hi part.
; ------------------------------------------------

; CHECK-LABEL: @test_generic
; CHECK:  [[ADDR:%[A-z0-9]*]] = ptrtoaddr ptr addrspace(4) %p to i64
; CHECK:  [[VEC:%[A-z0-9]*]] = bitcast i64 [[ADDR]] to <2 x i32>
; CHECK:  [[LO:%[A-z0-9]*]] = extractelement <2 x i32> [[VEC]], i32 0
; CHECK:  [[HI:%[A-z0-9]*]] = extractelement <2 x i32> [[VEC]], i32 1
; CHECK:  [[VEC0:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[LO]], i32 0
; CHECK:  [[VEC1:%[A-z0-9]*]] = insertelement <2 x i32> [[VEC0]], i32 [[HI]], i32 1
; CHECK:  store <2 x i32> [[VEC1]], ptr addrspace(1) %out

define spir_kernel void @test_generic(ptr addrspace(4) %p, ptr addrspace(1) %out) {
  %1 = ptrtoaddr ptr addrspace(4) %p to i64
  store i64 %1, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: @test_global
; CHECK:  [[ADDR:%[A-z0-9]*]] = ptrtoaddr ptr addrspace(1) %p to i64
; CHECK:  [[VEC:%[A-z0-9]*]] = bitcast i64 [[ADDR]] to <2 x i32>
; CHECK:  [[LO:%[A-z0-9]*]] = extractelement <2 x i32> [[VEC]], i32 0
; CHECK:  [[HI:%[A-z0-9]*]] = extractelement <2 x i32> [[VEC]], i32 1
; CHECK:  [[VEC0:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[LO]], i32 0
; CHECK:  [[VEC1:%[A-z0-9]*]] = insertelement <2 x i32> [[VEC0]], i32 [[HI]], i32 1
; CHECK:  store <2 x i32> [[VEC1]], ptr addrspace(1) %out

define spir_kernel void @test_global(ptr addrspace(1) %p, ptr addrspace(1) %out) {
  %1 = ptrtoaddr ptr addrspace(1) %p to i64
  store i64 %1, ptr addrspace(1) %out
  ret void
}

!igc.functions = !{!0, !3}

!0 = !{ptr @test_generic, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @test_global, !1}
