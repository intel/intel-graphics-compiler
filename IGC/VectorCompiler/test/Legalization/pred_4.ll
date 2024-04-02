;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | \
; RUN:   FileCheck %s --check-prefix=CHECK-XeHPC
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | \
; RUN:   FileCheck %s --check-prefix=CHECK-XeLP

declare i1 @llvm.genx.all.v8i1(<8 x i1>)
declare <8 x i64> @llvm.genx.rdregioni.v8i64.v32i64.i16(<32 x i64>, i32, i32, i32, i16, i32)

; CHECK-XeHPC-NOT: call <8 x i1> @llvm.genx.wrpredregion
; CHECK-XeHPC: icmp eq <8 x i64>

; CHECK-XeLP: [[CMP_SPLIT_0:%[^ ]+]] = icmp eq <4 x i64> %{{.*}}, <i64 4, i64 4, i64 4, i64 4>
; CHECK-XeLP: [[CMP_SPLIT_JOIN_0:%[^ ]+]] = call <8 x i1> @llvm.genx.wrpredregion.v8i1.v4i1(<8 x i1> undef, <4 x i1> [[CMP_SPLIT_0]], i32 0)
; CHECK-XeLP: [[CMP_SPLIT_1:%[^ ]+]] = icmp eq <4 x i64> %{{.*}}, <i64 4, i64 4, i64 4, i64 4>
; CHECK-XeLP: call <8 x i1> @llvm.genx.wrpredregion.v8i1.v4i1(<8 x i1> [[CMP_SPLIT_JOIN_0]], <4 x i1> [[CMP_SPLIT_1]], i32 4)

define spir_func void @foo(<32 x i64> %src)  {
  %rdreg = call <8 x i64> @llvm.genx.rdregioni.v8i64.v32i64.i16(<32 x i64> %src, i32 16, i32 4, i32 1, i16 0, i32 undef)
  %cmp = icmp eq <8 x i64> %rdreg, <i64 4, i64 4, i64 4, i64 4, i64 4, i64 4, i64 4, i64 4>
  %all = tail call i1 @llvm.genx.all.v8i1(<8 x i1> %cmp)
  ret void
}
