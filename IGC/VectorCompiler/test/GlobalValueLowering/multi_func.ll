;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@array_foo = internal global [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4
@array_bar = internal global [8 x i32] [i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50], align 4
@array_all = internal global [8 x i32] [i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51], align 4

; CHECK-LABEL: define dllexport spir_kernel void @multi_func
define dllexport spir_kernel void @multi_func(i64 %provided.offset) #1 {
; COM: all the lowered globals are at the function entry
; CHECK-DAG: %[[KERN_ALL_GADDR:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_all)
; CHECK-DAG: %[[KERN_FOO_GADDR:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_foo)

  %all.ptrtoint = ptrtoint [8 x i32]* @array_all to i64
  %all.user = add i64 %all.ptrtoint, 3
; CHECK: %all.user = add i64 %[[KERN_ALL_GADDR]], 3

  %foo.ptrtoint = ptrtoint [8 x i32]* @array_foo to i64
  %foo.user = add i64 %foo.ptrtoint, 5
; CHECK: %foo.user = add i64 %[[KERN_FOO_GADDR]], 5

  call void @foo(i64 %provided.offset)
  call void @bar(i64 %provided.offset)
  ret void
}

; CHECK-LABEL: define internal spir_func void @foo
define internal spir_func void @foo(i64 %provided.offset) {
; CHECK: %array_foo.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_foo)

  %foo.ptrtoint = ptrtoint [8 x i32]* @array_foo to i64
  %foo.user = add i64 %foo.ptrtoint, 7
; CHECK: %foo.user = add i64 %array_foo.gaddr, 7

  ret void
}

; CHECK-LABEL: define internal spir_func void @bar
define internal spir_func void @bar(i64 %provided.offset) {
; CHECK: %[[BAR_GADDR:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_bar)

  %bar.ptrtoint = ptrtoint [8 x i32]* @array_bar to i64
  %bar.user = add i64 %bar.ptrtoint, 9
; CHECK: %bar.user = add i64 %[[BAR_GADDR]], 9

  ret void
}

attributes #1 = { noinline nounwind "CMGenxMain" }
