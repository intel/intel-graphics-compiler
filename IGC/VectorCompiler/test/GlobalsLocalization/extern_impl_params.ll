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

; COM: Fake functions to obtain implicit args inside extern/indirect functions.
; COM: In real case scenario there'll be some code in place of those functions,
; COM: e.g r0 -> and -> i2p -> gep -> load. But the code itself doesn't affect
; COM: global variable lowering flow.
declare <3 x i16> @get_loc_id()
declare <3 x i32> @get_loc_sz()
declare <3 x i32> @get_grp_cnt()
declare i64 @get_printf_ptr()

; COM: Local size is used both in external and internal function.
define dllexport spir_kernel void @loc_sz_overlap(i64 %privBase, <3 x i32> %__arg_llvm.genx.local.size) #1 {
; CHECK-LABEL: define dllexport spir_kernel void @loc_sz_overlap
; CHECK: %__imparg_llvm.genx.local.size.local = alloca <3 x i32>
; CHECK-NOT: alloca

  store <3 x i32> %__arg_llvm.genx.local.size, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK: store <3 x i32> %__arg_llvm.genx.local.size, <3 x i32>* %__imparg_llvm.genx.local.size.local

  call void @internal_loc_sz()
; CHECK: %__imparg_llvm.genx.local.size.val = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.local.size.local
; CHECK: call {{.*}} @internal_loc_sz(<3 x i32> %__imparg_llvm.genx.local.size.val)

  call void @external_loc_size_loc_id()
; CHECK: call void @external_loc_size_loc_id()
  ret void
}

define spir_func void @external_loc_size_loc_id() #0 {
; CHECK-LABEL: define spir_func void @external_loc_size_loc_id
; CHECK-DAG: %__imparg_llvm.genx.local.size.local = alloca <3 x i32>
; CHECK-DAG: %__imparg_llvm.genx.local.id16.local = alloca <3 x i16>
; CHECK-NOT: alloca

  %elsli.get.loc.id = call <3 x i16> @get_loc_id()
  %elsli.get.loc.sz = call <3 x i32> @get_loc_sz()
  store <3 x i16> %elsli.get.loc.id, <3 x i16>* @__imparg_llvm.genx.local.id16
  store <3 x i32> %elsli.get.loc.sz, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK: store <3 x i16> %elsli.get.loc.id, <3 x i16>* %__imparg_llvm.genx.local.id16.local
; CHECK: store <3 x i32> %elsli.get.loc.sz, <3 x i32>* %__imparg_llvm.genx.local.size.local

  %elsli.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  %elsli.loc.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK: %elsli.loc.id = load <3 x i16>, <3 x i16>* %__imparg_llvm.genx.local.id16.local
; CHECK: %elsli.loc.sz = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.local.size.local
  ret void
}

define dllexport spir_kernel void @with_indir(i64 %privBase) #1 {
; CHECK-LABEL: define dllexport spir_kernel void @with_indir
; CHECK-NOT: alloca

  call void @external_indir_use()
  call void @external_mixed_use()
; CHECK: call void @external_indir_use()
; CHECK: call void @external_mixed_use()
  ret void
}

define spir_func void @external_indir_use() #0 {
; CHECK-LABEL: define spir_func void @external_indir_use
; CHECK-DAG: %__imparg_llvm.genx.local.size.local = alloca <3 x i32>
; CHECK-DAG: %__imparg_llvm.genx.local.id16.local = alloca <3 x i16>
; CHECK-NOT: alloca

  %eiu.get.loc.id = call <3 x i16> @get_loc_id()
  %eiu.get.loc.sz = call <3 x i32> @get_loc_sz()
  store <3 x i16> %eiu.get.loc.id, <3 x i16>* @__imparg_llvm.genx.local.id16
  store <3 x i32> %eiu.get.loc.sz, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK: store <3 x i16> %eiu.get.loc.id, <3 x i16>* %__imparg_llvm.genx.local.id16.local
; CHECK: store <3 x i32> %eiu.get.loc.sz, <3 x i32>* %__imparg_llvm.genx.local.size.local

  call void @internal_loc_sz()
; CHECK: %__imparg_llvm.genx.local.size.val = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.local.size.local
; CHECK: call {{.*}} @internal_loc_sz(<3 x i32> %__imparg_llvm.genx.local.size.val)

  call void @internal_loc_id()
; CHECK: %__imparg_llvm.genx.local.id16.val = load <3 x i16>, <3 x i16>* %__imparg_llvm.genx.local.id16.local
; CHECK: call {{.*}} @internal_loc_id(<3 x i16> %__imparg_llvm.genx.local.id16.val)
  ret void
}

define spir_func void @external_mixed_use() #0 {
; CHECK-LABEL: define spir_func void @external_mixed_use
; CHECK-DAG: %__imparg_llvm.vc.internal.print.buffer.local = alloca i64
; CHECK-DAG: %__imparg_llvm.genx.group.count.local = alloca <3 x i32>

  %emu.get.printf.ptr = call i64 @get_printf_ptr()
  %emu.get.grp.cnt = call <3 x i32> @get_grp_cnt()
  store i64 %emu.get.printf.ptr, i64* @__imparg_llvm.vc.internal.print.buffer
  store <3 x i32> %emu.get.grp.cnt, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK: store i64 %emu.get.printf.ptr, i64* %__imparg_llvm.vc.internal.print.buffer.local
; CHECK: store <3 x i32> %emu.get.grp.cnt, <3 x i32>* %__imparg_llvm.genx.group.count.local

  %emu.printf.ptr = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK: %emu.printf.ptr = load i64, i64* %__imparg_llvm.vc.internal.print.buffer.local

  call void @internal_grp_cnt()
; CHECK: %__imparg_llvm.genx.group.count.val = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.group.count.local
; CHECK: call {{.*}} @internal_grp_cnt(<3 x i32> %__imparg_llvm.genx.group.count.val)
  ret void
}

define internal spir_func void @internal_loc_sz() #0 {
; CHECK-LABEL: define internal spir_func {{.*}} @internal_loc_sz(<3 x i32> %__imparg_llvm.genx.local.size.in)
; CHECK: %__imparg_llvm.genx.local.size.local = alloca <3 x i32>
; CHECK: store <3 x i32> %__imparg_llvm.genx.local.size.in, <3 x i32>* %__imparg_llvm.genx.local.size.local

  %ils.loc.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK: %ils.loc.sz = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.local.size.local
  ret void
}

define internal spir_func void @internal_loc_id() #0 {
; CHECK-LABEL: define internal spir_func {{.*}} @internal_loc_id(<3 x i16> %__imparg_llvm.genx.local.id16.in)
; CHECK: %__imparg_llvm.genx.local.id16.local = alloca <3 x i16>
; CHECK: store <3 x i16> %__imparg_llvm.genx.local.id16.in, <3 x i16>* %__imparg_llvm.genx.local.id16.local

  %ili.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK: %ili.loc.id = load <3 x i16>, <3 x i16>* %__imparg_llvm.genx.local.id16.local
  ret void
}

define internal spir_func void @internal_grp_cnt() #0 {
; CHECK-LABEL: define internal spir_func {{.*}} @internal_grp_cnt(<3 x i32> %__imparg_llvm.genx.group.count.in)
; CHECK: %__imparg_llvm.genx.group.count.local = alloca <3 x i32>
; CHECK: store <3 x i32> %__imparg_llvm.genx.group.count.in, <3 x i32>* %__imparg_llvm.genx.group.count.local

  %igc.loc.id = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK: %igc.loc.id = load <3 x i32>, <3 x i32>* %__imparg_llvm.genx.group.count.local
  ret void
}

attributes #0 = { "target-cpu"="Gen9" }
attributes #1 = { "RequiresImplArgsBuffer" "target-cpu"="Gen9" }

!genx.kernels = !{!1}
!genx.kernel.internal = !{!3}

; COM: Arg Kind map: local_size -> 8, group_count -> 16, local_id -> 24, printf_buffer -> 88,
; COM:               private_base -> 96
!0 = !{}
!1 = !{void (i64, <3 x i32>)* @loc_sz_overlap, !"loc_sz_overlap", !2, i32 0, !0, !0, !0, i32 0, i32 0}
!2 = !{i32 96, i32 8}
!3 = !{void (i64, <3 x i32>)* @loc_sz_overlap, null, null, !0, null}
!4 = !{void (i64)* @with_indir, !"with_indir", !5, i32 0, !0, !0, !0, i32 0, i32 0}
!5 = !{i32 96}
!6 = !{void (i64)* @with_indir, null, null, !0, null}
