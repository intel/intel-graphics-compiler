;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
;
; RUN: igc_opt --regkey "EnableOpaquePointersBackend=1" --igc-predefined-constant-resolve 2>&1 -S < %s | FileCheck %s
; ------------------------------------------------
; PredefinedConstantResolving
; ------------------------------------------------

; This test checks whether the PredefinedConstantResolving pass correctly folds constants with matching or not matching types

@global_int = internal addrspace(1) constant i32 1337, align 4
@global_i8s_arr = internal addrspace(1) constant [8 x i8] [i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8], align 8
@global_i64s_arr = internal addrspace(1) constant [3 x i64] [i64 16, i64 32, i64 64], align 8

; CHECK-LABEL: @test_i64_from_i64_array(
; CHECK: store i64 16,
; CHECK-NOT: load
define spir_kernel void @test_i64_from_i64_array(ptr addrspace(1) %out) {
entry:
  %local = load i64, ptr addrspace(1) @global_i64s_arr, align 8
  store i64 %local, ptr addrspace(1) %out, align 8
  ret void
}

; CHECK-LABEL: @test_direct_i32(
; CHECK: store i32 1337,
; CHECK-NOT: load
define spir_kernel void @test_direct_i32(ptr addrspace(1) %out) {
entry:
  %local = load i32, ptr addrspace(1) @global_int
  store i32 %local, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: @test_i64_from_i8_array(
; CHECK: store i64 578437695752307201,
; CHECK-NOT: load
define spir_kernel void @test_i64_from_i8_array(ptr addrspace(1) %out) {
entry:
  %local = load i64, ptr addrspace(1) @global_i8s_arr, align 8
  store i64 %local, ptr addrspace(1) %out, align 8
  ret void
}

; CHECK-LABEL: @test_i8_from_i64_array(
; CHECK: store i8 16,
; CHECK-NOT: load
define spir_kernel void @test_i8_from_i64_array(ptr addrspace(1) %out) {
entry:
  %local = load i8, ptr addrspace(1) @global_i64s_arr, align 1
  store i8 %local, ptr addrspace(1) %out, align 1
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" "visaStackCall" }