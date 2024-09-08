;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

define void @fence_acq() {
  ; CHECK: call void @llvm.genx.fence(i8 65)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 2, i8 4)
  fence syncscope("all_devices") acquire
  ret void
}

define void @fence_rel() {
  ; CHECK: call void @llvm.genx.fence(i8 1)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 4, i8 2)
  fence syncscope("device") release
  ret void
}

define void @fence_acq_rel() {
  ; CHECK: call void @llvm.genx.fence(i8 65)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 1, i8 2)
  fence syncscope("device") acq_rel
  ret void
}

define void @fence_acq_rel_wg() {
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  fence syncscope("workgroup") acq_rel
  ret void
}

define void @fence_seq_cst() {
  ; CHECK-NOT: call void @llvm.genx.fence
  ; CHECK-LSC-NOT: call void @llvm.genx.lsc.fence.i1
  fence syncscope("subgroup") seq_cst
  ret void
}

define void @fence_workitem() {
  ; CHECK-NOT: call void @llvm.genx.fence
  ; CHECK-LSC-NOT: call void @llvm.genx.lsc.fence
  fence syncscope("workitem") seq_cst
  ret void
}
