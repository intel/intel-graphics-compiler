;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; This test verifies that the PreprocessSPVIR pass removes @llvm.ptr.annotation
; intrinsic calls and replaces their uses with the original annotated pointer.

@.str = private unnamed_addr constant [15 x i8] c"my_annotation\00\00", align 1

; CHECK-LABEL: define spir_kernel void @test_remove_ptr_annotation
; CHECK: [[PTR:%.*]] = alloca {{.*}}
; CHECK: store i32 1, ptr [[PTR]], align 4
; CHECK-NOT: call {{.*}} @llvm.ptr.annotation

define spir_kernel void @test_remove_ptr_annotation() {
  %1 = alloca i32, align 4
  %2 = call ptr @llvm.ptr.annotation.p0.p0(ptr %1, ptr @.str, ptr undef, i32 undef, ptr undef)
  store i32 1, ptr %2, align 4
  ret void
}

declare ptr @llvm.ptr.annotation.p0.p0(ptr, ptr, ptr, i32, ptr)
