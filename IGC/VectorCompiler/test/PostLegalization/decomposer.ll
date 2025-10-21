;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -dce -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

; target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
; target triple = "genx64-unknown-unknown"

; CHECK-LABEL: test
define spir_kernel void @test() {
entry:
  br label %._crit_edge

._crit_edge:                                      ; preds = %.lr.phentry, %entry
; CHECK: {{.*}}decomp.0 = phi <32 x i32> [ zeroinitializer, %entry ], [ [[WRREG1:%[^ ]+]], %.lr.phentry ]
; CHECK: {{.*}}decomp.1 = phi <32 x i32> [ zeroinitializer, %entry ], [ [[WRREG2:%[^ ]+]], %.lr.phentry ]
  %.sroa.0100.0.lcssa = phi <64 x i32> [ zeroinitializer, %entry ], [ %.esimd639.join32, %.lr.phentry ]
  %.esimd6 = tail call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %.sroa.0100.0.lcssa, i32 0, i32 0, i32 1, i16 128, i32 0)
  %.esimd8.regioncollapsed = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.v32i1(<64 x i32> %.sroa.0100.0.lcssa, <32 x i32> %.esimd6, i32 0, i32 0, i32 1, i16 0, i32 0, <32 x i1> zeroinitializer)
  %.esimd57 = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v64i32.i16(<64 x i32> %.esimd8.regioncollapsed, i32 0, i32 0, i32 0, i16 0, i32 0)
  tail call void @llvm.genx.scatter.scaled.v1i1.v1i32.v1i32(<1 x i1> zeroinitializer, i32 0, i16 0, i32 0, i32 0, <1 x i32> zeroinitializer, <1 x i32> %.esimd57)
  br label %.lr.ph

.lr.ph:                                           ; preds = %._crit_edge
  %.split016 = or <32 x i32> zeroinitializer, zeroinitializer
  %.esimd639.join0 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.v32i1(<64 x i32> zeroinitializer, <32 x i32> %.split016, i32 0, i32 0, i32 0, i16 0, i32 0, <32 x i1> zeroinitializer)
; CHECK: [[WRREG1]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v32i32.i16.v32i1(<32 x i32> zeroinitializer, <32 x i32> %.split016, i32 0, i32 1, i32 0, i16 0, i32 undef, <32 x i1> zeroinitializer)
  %.split3218 = or <32 x i32> zeroinitializer, zeroinitializer
  %.esimd639.join32 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.v32i1(<64 x i32> %.esimd639.join0, <32 x i32> %.split3218, i32 0, i32 0, i32 0, i16 128, i32 0, <32 x i1> zeroinitializer)
; CHECK: [[WRREG2]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v32i32.i16.v32i1(<32 x i32> zeroinitializer, <32 x i32> %.split3218, i32 0, i32 1, i32 0, i16 0, i32 undef, <32 x i1> zeroinitializer)
  br label %.lr.phentry

.lr.phentry:                     ; preds = %.lr.ph
  br label %._crit_edge
}

declare <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)
declare <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.v32i1(<64 x i32>, <32 x i32>, i32, i32, i32, i16, i32, <32 x i1>)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)
declare void @llvm.genx.scatter.scaled.v1i1.v1i32.v1i32(<1 x i1>, i32, i16, i32, i32, <1 x i32>, <1 x i32>)
