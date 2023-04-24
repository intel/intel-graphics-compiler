;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=true -march=genx64 -mcpu=Gen9 -S < %s \
; RUN:    | FileCheck --check-prefix=CM-CHECK %s
; RUN: %not_for_vc_diag% opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -march=genx64 -mcpu=Gen9 -S \
; RUN:    < %s 2>&1 | FileCheck --check-prefix=OCL-CHECK %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; OCL-CHECK: scoreboarding intrinsics are supported only for CM RT

; CM-CHECK-DAG: @__imparg_llvm.genx.get.scoreboard.bti = internal global i32 undef
; CM-CHECK-DAG: @__imparg_llvm.genx.get.scoreboard.depcnt = internal global i32 undef
; CM-CHECK-DAG: @__imparg_llvm.genx.get.scoreboard.deltas = internal global <16 x i8> undef

declare i32 @llvm.genx.get.scoreboard.bti()
declare i32 @llvm.genx.get.scoreboard.depcnt()
declare <16 x i8> @llvm.genx.get.scoreboard.deltas()

define dllexport spir_kernel void @kernel() {
; CM-CHECK-LABEL: define dllexport spir_kernel void @kernel(
; CM-CHECK-DAG: i32 %impl.arg.llvm.genx.get.scoreboard.bti
; CM-CHECK-DAG: i32 %impl.arg.llvm.genx.get.scoreboard.depcnt
; CM-CHECK-DAG: <16 x i8> %impl.arg.llvm.genx.get.scoreboard.deltas
; CM-CHECK-SAME: )

; COM: Check saving implicit args at the beginning of the kernel.
; CM-CHECK-DAG: store i32 %impl.arg.llvm.genx.get.scoreboard.bti, i32* @__imparg_llvm.genx.get.scoreboard.bti
; CM-CHECK-DAG: store i32 %impl.arg.llvm.genx.get.scoreboard.depcnt, i32* @__imparg_llvm.genx.get.scoreboard.depcnt
; CM-CHECK-DAG: store <16 x i8> %impl.arg.llvm.genx.get.scoreboard.deltas, <16 x i8>* @__imparg_llvm.genx.get.scoreboard.deltas

  %k.bti = call i32 @llvm.genx.get.scoreboard.bti()
  %k.depcnt = call i32 @llvm.genx.get.scoreboard.depcnt()
  %k.deltas = call <16 x i8> @llvm.genx.get.scoreboard.deltas()
; CM-CHECK: %k.bti = load i32, i32* @__imparg_llvm.genx.get.scoreboard.bti
; CM-CHECK: %k.depcnt = load i32, i32* @__imparg_llvm.genx.get.scoreboard.depcnt
; CM-CHECK: %k.deltas = load <16 x i8>, <16 x i8>* @__imparg_llvm.genx.get.scoreboard.deltas

  ret void
}

!genx.kernels = !{!1}
; CM-CHECK: !genx.kernels = !{![[KERN_MD:[0-9]+]]}

!0 = !{}
!1 = !{void ()* @kernel, !"kernel", !0, i32 0, !0, !0, !0, i32 0, i32 0}
; COM: Arg Kind map: bti -> 42, depcnt -> 50, deltas -> 32
; CM-CHECK: ![[KERN_MD]] = !{void ({{.*}})* @kernel, !"kernel", ![[KERN_AK_MD:[0-9]+]]
; CM-CHECK: ![[KERN_AK_MD]] = !{
; CM-CHECK-DAG: i32 42
; CM-CHECK-DAG: i32 50
; CM-CHECK-DAG: i32 32
; CM-CHECK-SAME: }
