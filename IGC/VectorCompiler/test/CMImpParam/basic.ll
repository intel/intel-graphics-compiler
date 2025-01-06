;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK-DAG: @__imparg_llvm.genx.local.id16 = internal global <3 x i16> undef
; CHECK-DAG: @__imparg_llvm.genx.local.size = internal global <3 x i32> undef
; CHECK-DAG: @__imparg_llvm.genx.group.count = internal global <3 x i32> undef
; CHECK-DAG: @__imparg_llvm.vc.internal.print.buffer = internal global i64 undef
; CHECK-DAG: @__imparg_llvm.vc.internal.assert.buffer = internal global i64 undef
; CHECK-DAG: @__imparg_llvm.vc.internal.sync.buffer = internal global i64 undef

declare <3 x i16> @llvm.genx.local.id16.v3i16()
declare <3 x i32> @llvm.genx.local.size.v3i32()
declare i32 @llvm.genx.group.id.x()
declare i32 @llvm.genx.group.id.y()
declare i32 @llvm.genx.group.id.z()
declare <3 x i32> @llvm.genx.group.count.v3i32()
declare i64 @llvm.vc.internal.print.buffer()
declare i64 @llvm.vc.internal.assert.buffer()
declare i64 @llvm.vc.internal.sync.buffer()

define dllexport spir_kernel void @direct() {
; CHECK: define dllexport spir_kernel void @direct(
; CHECK-DAG: <3 x i16> %impl.arg.llvm.genx.local.id16
; CHECK-DAG: <3 x i32> %impl.arg.llvm.genx.local.size
; CHECK-DAG: <3 x i32> %impl.arg.llvm.genx.group.count
; CHECK-DAG: i64 %impl.arg.llvm.vc.internal.print.buffer
; CHECK-DAG: i64 %impl.arg.private.base
; CHECK-SAME: ) #[[KERN_ATTR:[0-9]+]] {

; COM: Check saving implicit args at the beginning of the kernel.
; CHECK-TYPED-PTRS-DAG: store <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-TYPED-PTRS-DAG: store <3 x i32> %impl.arg.llvm.genx.local.size, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-TYPED-PTRS-DAG: store <3 x i32> %impl.arg.llvm.genx.group.count, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-TYPED-PTRS-DAG: store i64 %impl.arg.llvm.vc.internal.print.buffer, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS-DAG: store <3 x i16> %impl.arg.llvm.genx.local.id16, ptr @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS-DAG: store <3 x i32> %impl.arg.llvm.genx.local.size, ptr @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS-DAG: store <3 x i32> %impl.arg.llvm.genx.group.count, ptr @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS-DAG: store i64 %impl.arg.llvm.vc.internal.print.buffer, ptr @__imparg_llvm.vc.internal.print.buffer

  %d.loc.id = call <3 x i16> @llvm.genx.local.id16.v3i16()
  %d.loc.sz = call <3 x i32> @llvm.genx.local.size.v3i32()
; CHECK-TYPED-PTRS: %d.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-TYPED-PTRS: %d.loc.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS: %d.loc.id = load <3 x i16>, ptr @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: %d.loc.sz = load <3 x i32>, ptr @__imparg_llvm.genx.local.size

  %d.grp.x = call i32 @llvm.genx.group.id.x()
  %d.grp.y = call i32 @llvm.genx.group.id.y()
  %d.grp.z = call i32 @llvm.genx.group.id.z()
; COM: Group IDs don't require implicit args.
; CHECK: %d.grp.x = call i32 @llvm.genx.group.id.x()
; CHECK: %d.grp.y = call i32 @llvm.genx.group.id.y()
; CHECK: %d.grp.z = call i32 @llvm.genx.group.id.z()

  %d.grp.sz = call <3 x i32> @llvm.genx.group.count.v3i32()
  %d.print = call i64 @llvm.vc.internal.print.buffer()
  %d.assert = call i64 @llvm.vc.internal.assert.buffer()
  %d.sync = call i64 @llvm.vc.internal.sync.buffer()
; CHECK-TYPED-PTRS: %d.grp.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-TYPED-PTRS: %d.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-TYPED-PTRS: %d.assert = load i64, i64* @__imparg_llvm.vc.internal.assert.buffer
; CHECK-TYPED-PTRS: %d.sync = load i64, i64* @__imparg_llvm.vc.internal.sync.buffer
; CHECK-OPAQUE-PTRS: %d.grp.sz = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS: %d.print = load i64, ptr @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS: %d.assert = load i64, ptr @__imparg_llvm.vc.internal.assert.buffer
; CHECK-OPAQUE-PTRS: %d.sync = load i64, ptr @__imparg_llvm.vc.internal.sync.buffer
  ret void
}

define dllexport spir_kernel void @direct_partial() {
; CHECK: define dllexport spir_kernel void @direct_partial(
; CHECK-DAG: <3 x i32> %impl.arg.llvm.genx.local.size
; CHECK-DAG: <3 x i32> %impl.arg.llvm.genx.group.count
; CHECK-SAME: ) #[[KERN_ATTR]] {

  %dp.loc.sz = call <3 x i32> @llvm.genx.local.size.v3i32()
  %dp.grp.sz = call <3 x i32> @llvm.genx.group.count.v3i32()
; CHECK-TYPED-PTRS: %dp.loc.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS: %dp.grp.sz = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
  ret void
}

; COM: CMImplParam traverses called internal functions and collects requred
; COM: implicit arguments.
define dllexport spir_kernel void @indir() {
; CHECK: define dllexport spir_kernel void @indir(
; CHECK-SAME: <3 x i32> %impl.arg.llvm.genx.group.count
; CHECK-SAME: <3 x i16> %impl.arg.llvm.genx.local.id16
; CHECK-SAME: i64 %impl.arg.llvm.vc.internal.assert.buffer
; CHECK-SAME: i64 %impl.arg.llvm.vc.internal.print.buffer
; CHECK-SAME: i64 %impl.arg.llvm.vc.internal.sync.buffer
; CHECK-SAME: ) #[[KERN_ATTR]] {
  call void @indir_func_1()
  call void @indir_func_2()
  call void @indir_func_3()
  ret void
}

define internal spir_func void @indir_func_1() {
  %i.1.loc.id = call <3 x i16> @llvm.genx.local.id16.v3i16()
; CHECK-TYPED-PTRS: %i.1.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: %i.1.loc.id = load <3 x i16>, ptr @__imparg_llvm.genx.local.id16
  ret void
}

define internal spir_func void @indir_func_2() {
  %i.2.loc.id = call <3 x i16> @llvm.genx.local.id16.v3i16()
  %i.2.grp.sz = call <3 x i32> @llvm.genx.group.count.v3i32()
; CHECK-TYPED-PTRS: %i.2.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-TYPED-PTRS: %i.2.grp.sz = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS: %i.2.loc.id = load <3 x i16>, ptr @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: %i.2.grp.sz = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
  call void @indir_func_common()
  ret void
}

define internal spir_func void @indir_func_3() {
  call void @indir_func_common()
  ret void
}

define internal spir_func void @indir_func_common() {
  %i.c.print = call i64 @llvm.vc.internal.print.buffer()
  %i.c.assert = call i64 @llvm.vc.internal.assert.buffer()
  %i.c.sync = call i64 @llvm.vc.internal.sync.buffer()
; CHECK-TYPED-PTRS: %i.c.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-TYPED-PTRS: %i.c.assert = load i64, i64* @__imparg_llvm.vc.internal.assert.buffer
; CHECK-TYPED-PTRS: %i.c.sync = load i64, i64* @__imparg_llvm.vc.internal.sync.buffer
; CHECK-OPAQUE-PTRS: %i.c.print = load i64, ptr @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS: %i.c.assert = load i64, ptr @__imparg_llvm.vc.internal.assert.buffer
; CHECK-OPAQUE-PTRS: %i.c.sync = load i64, ptr @__imparg_llvm.vc.internal.sync.buffer
  ret void
}

; CHECK: attributes #[[KERN_ATTR]] = {
; CHECK-NOT: "RequiresImplArgsBuffer"
; CHECK-SAME: }

!genx.kernels = !{!0, !2, !3}
; CHECK: !genx.kernels = !{![[D_KERN_MD:[0-9]+]], ![[DP_KERN_MD:[0-9]+]], ![[I_KERN_MD:[0-9]+]]}

!0 = !{void ()* @direct, !"direct", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
!2 = !{void ()* @direct_partial, !"direct_partial", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!3 = !{void ()* @indir, !"indir", !1, i32 0, !1, !1, !1, i32 0, i32 0}
; COM: Arg Kind map: local_size -> 8, group_count -> 16, local_id -> 24, printf_buffer -> 88,
; COM:               private_base -> 96, sync_buffer -> 184
; CHECK-TYPED-PTRS: ![[D_KERN_MD]] = !{void ({{.*}})* @direct, !"direct", ![[D_KERN_AK_MD:[0-9]+]]
; CHECK-OPAQUE-PTRS: ![[D_KERN_MD]] = !{ptr @direct, !"direct", ![[D_KERN_AK_MD:[0-9]+]]
; CHECK: ![[D_KERN_AK_MD]] = !{
; CHECK-DAG: i32 8
; CHECK-DAG: i32 16
; CHECK-DAG: i32 24
; CHECK-DAG: i32 88
; CHECK-DAG: i32 96
; CHECK-DAG: i32 184
; CHECK: }
; CHECK-TYPED-PTRS: ![[DP_KERN_MD]] = !{void ({{.*}})* @direct_partial, !"direct_partial", ![[DP_KERN_AK_MD:[0-9]+]]
; CHECK-OPAQUE-PTRS: ![[DP_KERN_MD]] = !{ptr @direct_partial, !"direct_partial", ![[DP_KERN_AK_MD:[0-9]+]]
; CHECK: ![[DP_KERN_AK_MD]] = !{
; CHECK-NOT: i32 24
; CHECK-NOT: i32 88
; CHECK: }
; CHECK-TYPED-PTRS: ![[I_KERN_MD]] = !{void ({{.*}})* @indir, !"indir", ![[I_KERN_AK_MD:[0-9]+]]
; CHECK-OPAQUE-PTRS: ![[I_KERN_MD]] = !{ptr @indir, !"indir", ![[I_KERN_AK_MD:[0-9]+]]
; CHECK: ![[I_KERN_AK_MD]] = !{
; CHECK-COUNT-6: i32 {{[0-9]+}}
; CHECK-NOT: i32 {{[0-9]+}}
; CHECK: }
