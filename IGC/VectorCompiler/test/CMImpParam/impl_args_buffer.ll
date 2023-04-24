;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare i32 @llvm.genx.absi.i32(i32)
declare spir_func <3 x i32> @llvm.genx.local.id.v3i32()
declare spir_func i32 @some_extern_func()

define dllexport spir_kernel void @with_genx_intr() {
; CHECK: @with_genx_intr(
; CHECK-SAME: ) #[[NO_BUFFER_ATTR:[0-9]+]] {
  %genx.intr = call i32 @llvm.genx.absi.i32(i32 0)
  ret void
}

define dllexport spir_kernel void @with_extern_func() {
; CHECK: @with_extern_func(
; CHECK-SAME: ) #[[WITH_BUFFER_ATTR:[0-9]+]] {
  %extern.func = call spir_func i32 @some_extern_func()
  ret void
}

define dllexport spir_kernel void @with_indir_call() {
; CHECK: @with_indir_call(
; CHECK-SAME: ) #[[WITH_BUFFER_ATTR]] {
  %func.ptr = inttoptr i32 42 to i32 ()*
  %indir.call = call spir_func i32 %func.ptr()
  ret void
}

define spir_func void @empty_func() {
  ret void
}

define dllexport spir_kernel void @with_empty_func() {
; CHECK: @with_empty_func(
; CHECK-SAME: ) #[[NO_BUFFER_ATTR]] {
  call spir_func void @empty_func()
  ret void
}

define dllexport spir_kernel void @with_lid_func() {
; CHECK: @with_lid_func(
; CHECK-SAME: ) #[[WITH_BUFFER_ATTR]] {
  call spir_func void @local_id_func()
  ret void
}

define spir_func void @local_id_func() {
  %lif = call spir_func <3 x i32> @llvm.genx.local.id.v3i32()
  ret void
}

define dllexport spir_kernel void @with_inline_asm() {
; CHECK: @with_inline_asm(
; CHECK-SAME: ) #[[NO_BUFFER_ATTR]] {
  %asm = tail call <4 x i32> asm "mad (M1, 4) $0 $1 $2 $3", "=r,r,r,r"(<4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  ret void
}

; CHECK: attributes #[[NO_BUFFER_ATTR]] = {
; CHECK-NOT: "RequiresImplArgsBuffer"
; CHECK-SAME: }

; CHECK: attributes #[[WITH_BUFFER_ATTR]] = {
; CHECK-SAME: "RequiresImplArgsBuffer"
; CHECK-SAME: }

!genx.kernels = !{!1, !2, !3, !4, !5, !6}

!0 = !{}
!1 = !{void ()* @with_genx_intr, !"with_genx_intr", !0, i32 0, !0, !0, !0, i32 0, i32 0}
!2 = !{void ()* @with_extern_func, !"with_extern_func", !0, i32 0, !0, !0, !0, i32 0, i32 0}
!3 = !{void ()* @with_indir_call, !"with_indir_call", !0, i32 0, !0, !0, !0, i32 0, i32 0}
!4 = !{void ()* @with_empty_func, !"with_empty_func", !0, i32 0, !0, !0, !0, i32 0, i32 0}
!5 = !{void ()* @with_lid_func, !"with_lid_func", !0, i32 0, !0, !0, !0, i32 0, i32 0}
!6 = !{void ()* @with_inline_asm, !"with_inline_asm", !0, i32 0, !0, !0, !0, i32 0, i32 0}
