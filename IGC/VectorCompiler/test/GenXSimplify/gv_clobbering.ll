;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -genx-simplify -mcpu=Gen9 -march=genx64 -mtriple=spir64 -S < %s | \
; RUN:   FileCheck --check-prefix=FIXED %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@left2 = external global <8 x double> #0

declare <8 x double> @llvm.genx.rdregionf.v8f64.v8f64.i16(<8 x double>, i32, i32, i32, i16, i32)
declare <1 x double> @llvm.genx.rdregionf.v1f64.v8f64.i16(<8 x double>, i32, i32, i32, i16, i32)
declare <8 x double> @llvm.genx.vload.v8f64.p0v8f64(<8 x double>*)
declare void @llvm.genx.vstore.v8f64.p0v8f64(<8 x double>, <8 x double>*)

define spir_kernel void @wavefront_pth_3_atomic_noflag_r63_s2d_group__double_8_8_16() #1 {
  %gload47.i = call <8 x double> @llvm.genx.vload.v8f64.p0v8f64(<8 x double>* @left2)
  %gload47.i._gvload_legalized_rdr = call <8 x double> @llvm.genx.rdregionf.v8f64.v8f64.i16(<8 x double> %gload47.i, i32 0, i32 0, i32 1, i16 0, i32 0)
  call void @llvm.genx.vstore.v8f64.p0v8f64(<8 x double> zeroinitializer, <8 x double>* @left2)
  %rdr39.i = call <1 x double> @llvm.genx.rdregionf.v1f64.v8f64.i16(<8 x double> %gload47.i._gvload_legalized_rdr, i32 0, i32 0, i32 0, i16 0, i32 0)
  ret void
}

; FIXED: define spir_kernel void @wavefront_pth_3_atomic_noflag_r63_s2d_group__double_8_8_16() #2 {
; FIXED:  %gload47.i = call <8 x double> @llvm.genx.vload.v8f64.p0v8f64(<8 x double>* @left2)
; FIXED:  %gload47.i._gvload_legalized_rdr = call <8 x double> @llvm.genx.rdregionf.v8f64.v8f64.i16(<8 x double> %gload47.i, i32 0, i32 0, i32 1, i16 0, i32 0)
; FIXED:  call void @llvm.genx.vstore.v8f64.p0v8f64(<8 x double> zeroinitializer, <8 x double>* @left2)
; FIXED-NOT:  %rdr39.i = call <1 x double> @llvm.genx.rdregionf.v1f64.v8f64.i16(<8 x double> %gload47.i, i32 0, i32 0, i32 0, i16 0, i32 0)

attributes #0 = { "genx_volatile" }
attributes #1 = { "CMGenxMain" }
