;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

@__imparg_llvm.genx.local.id16 = internal global <3 x i16> undef
@__imparg_llvm.genx.local.size = internal global <3 x i32> undef
@__imparg_llvm.genx.group.count = internal global <3 x i32> undef
@__imparg_llvm.vc.internal.print.buffer = internal global i64 undef

define dllexport spir_kernel void @direct(i64 %privBase, <3 x i16> %__arg_llvm.genx.local.id16, <3 x i32> %__arg_llvm.genx.local.size, i64 %__arg_llvm.vc.internal.print.buffer, <3 x i32> %__arg_llvm.genx.group.count) #0 {
; CHECK-LABEL:  @direct
; CHECK-DAG: %__imparg_llvm.vc.internal.print.buffer.local = alloca i64
; CHECK-DAG: %__imparg_llvm.genx.group.count.local = alloca <3 x i32>
; CHECK-DAG: %__imparg_llvm.genx.local.size.local = alloca <3 x i32>
; CHECK-DAG: %__imparg_llvm.genx.local.id16.local = alloca <3 x i16>
; CHECK-NOT: alloca

  store <3 x i16> %__arg_llvm.genx.local.id16, <3 x i16>* @__imparg_llvm.genx.local.id16
  store <3 x i32> %__arg_llvm.genx.local.size, <3 x i32>* @__imparg_llvm.genx.local.size
  store i64 %__arg_llvm.vc.internal.print.buffer, i64* @__imparg_llvm.vc.internal.print.buffer
  store <3 x i32> %__arg_llvm.genx.group.count, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK: store <3 x i16> %__arg_llvm.genx.local.id16, <3 x i16>* %__imparg_llvm.genx.local.id16.local
; CHECK: store <3 x i32> %__arg_llvm.genx.local.size, <3 x i32>* %__imparg_llvm.genx.local.size.local
; CHECK: store i64 %__arg_llvm.vc.internal.print.buffer, i64* %__imparg_llvm.vc.internal.print.buffer.local
; CHECK: store <3 x i32> %__arg_llvm.genx.group.count, <3 x i32>* %__imparg_llvm.genx.group.count.local

  %d.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  %d.loc.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
  %d.grp.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
  %d.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK: %d.loc.id = load <3 x i16>, <3 x i16>* %__imparg_llvm.genx.local.id16.local
; CHECK: %d.loc.sz = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.local.size.local
; CHECK: %d.grp.sz = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.group.count.local
; CHECK: %d.print = load i64, i64* %__imparg_llvm.vc.internal.print.buffer.local

  ret void
}

define dllexport spir_kernel void @indir(i64 %privBase, i64 %__arg_llvm.vc.internal.print.buffer, <3 x i32> %__arg_llvm.genx.group.count, <3 x i16> %__arg_llvm.genx.local.id16) #0 {
; CHECK-LABEL: @indir
  store i64 %__arg_llvm.vc.internal.print.buffer, i64* @__imparg_llvm.vc.internal.print.buffer
  store <3 x i32> %__arg_llvm.genx.group.count, <3 x i32>* @__imparg_llvm.genx.group.count
  store <3 x i16> %__arg_llvm.genx.local.id16, <3 x i16>* @__imparg_llvm.genx.local.id16
  call void @indir_func_1()
  call void @indir_func_2()
  call void @indir_func_3()
  ret void
}

define internal spir_func void @indir_func_1() #0 {
; CHECK: define internal spir_func {{.*}} @indir_func_1(<3 x i16> %__imparg_llvm.genx.local.id16.in)
; CHECK: %__imparg_llvm.genx.local.id16.local = alloca <3 x i16>
; CHECK: store <3 x i16> %__imparg_llvm.genx.local.id16.in, <3 x i16>* %__imparg_llvm.genx.local.id16.local

  %i.1.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK: %i.1.loc.id = load <3 x i16>, <3 x i16>* %__imparg_llvm.genx.local.id16.local
  ret void
}

define internal spir_func void @indir_func_2() #0 {
; CHECK: define internal spir_func {{.*}} @indir_func_2(
; CHECK-DAG: <3 x i16> %__imparg_llvm.genx.local.id16.in,
; CHECK-DAG: <3 x i32> %__imparg_llvm.genx.group.count.in,
; CHECK-DAG: i64 %__imparg_llvm.vc.internal.print.buffer.in
; CHECK: ) #0 {
  %i.2.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  %i.2.grp.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
  call void @indir_func_common()
  ret void
}

define internal spir_func void @indir_func_3() #0 {
; CHECK: define internal spir_func {{.*}} @indir_func_3(i64 %__imparg_llvm.vc.internal.print.buffer.in)
  call void @indir_func_common()
  ret void
}

define internal spir_func void @indir_func_common() #0 {
; CHECK: define internal spir_func {{.*}} @indir_func_common(i64 %__imparg_llvm.vc.internal.print.buffer.in)
  %i.c.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
  ret void
}

attributes #0 = { "target-cpu"="Gen9" }

!genx.kernels = !{!0, !3}
!genx.kernel.internal = !{!5, !6}

!0 = !{void (i64, <3 x i16>, <3 x i32>, i64, <3 x i32>)* @direct, !"direct", !1, i32 0, !2, !2, !2, i32 0, i32 0}
!1 = !{i32 96, i32 24, i32 8, i32 88, i32 16}
!2 = !{}
!3 = !{void (i64, i64, <3 x i32>, <3 x i16>)* @indir, !"indir", !4, i32 0, !2, !2, !2, i32 0, i32 0}
!4 = !{i32 96, i32 88, i32 16, i32 24}
!5 = !{void (i64, <3 x i16>, <3 x i32>, i64, <3 x i32>)* @direct, null, null, !2, null}
!6 = !{void (i64, i64, <3 x i32>, <3 x i16>)* @indir, null, null, !2, null}
