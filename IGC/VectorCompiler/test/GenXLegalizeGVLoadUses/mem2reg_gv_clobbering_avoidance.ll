;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;=================================================
; This test checks that GenXLegalizeGVLoadUses will prevent mem2reg from optimizing out
;--------------- Before mem2reg  ------------------
; L = genx.vload(@GENX_VOLATILE_GLOBAL*)
; store(L, %SIMPLE_GLOBAL_LOCALIZED*) // LLVM's store
; // %SIMPLE_GLOBAL_LOCALIZED is not getting rewritten anywhere below and its type is the same as the type of @GENX_VOLATILE_GLOBAL
; genx.vstore(SOME_X, @GENX_VOLATILE_GLOBAL) // This is not accounted for by mem2reg, because LLVM semantics != VC BE semantics.
; L2 = load(%SIMPLE_GLOBAL_LOCALIZED*) // LLVM's load
; use(L2)
;--------------- After mem2reg -------------------
; // @GENX_VOLATILE_GLOBAL is used for @SIMPLE_GLOBAL because see comments above.
; // Result:
; L = vload(@GENX_VOLATILE_GLOBAL*)
; genx.vstore(SOME_X, @GENX_VOLATILE_GLOBAL) // Clobbering.
; use(L)
;=================================================
;
; RUN: %opt %use_old_pass_manager% -mem2reg -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | \
; RUN:     FileCheck --check-prefix=UNFIXED %s
;
; RUN: %opt %use_old_pass_manager% -GenXLegalizeGVLoadUses -mem2reg -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | \
; RUN:     FileCheck --check-prefix=FIXED %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@left2 = external global <1 x double> #0

declare <1 x double> @llvm.genx.rdregionf.v1f64.v1f64.i16(<1 x double>, i32, i32, i32, i16, i32)
declare <1 x double> @llvm.genx.vload.v1f64.p0v1f64(<1 x double>*)
declare void @llvm.genx.vstore.v1f64.p0v1f64(<1 x double>, <1 x double>*)

; Reduced from wavefront_pth_3_atomic_noflag_r63_s2d_group__double_8_8_16.
define spir_kernel void @test() #1 {
  %left.local = alloca <1 x double>, i32 0, align 64 ; In original testcase input this was a global variable, at this point it is "localized".
  %gload47.i = call <1 x double> @llvm.genx.vload.v1f64.p0v1f64(<1 x double>* @left2)
  store <1 x double> %gload47.i, <1 x double>* %left.local, align 64
  call void @llvm.genx.vstore.v1f64.p0v1f64(<1 x double> zeroinitializer, <1 x double>* @left2)
  %local_load = load <1 x double>, <1 x double>* %left.local, align 64
  %rdr39.i = call <1 x double> @llvm.genx.rdregionf.v1f64.v1f64.i16(<1 x double> %local_load, i32 0, i32 1, i32 0, i16 0, i32 0)
  ret void
}

; FIXED: define spir_kernel void @test()
; FIXED:  %gload47.i = call <1 x double> @llvm.genx.vload.v1f64.p0v1f64(<1 x double>* @left2)
; FIXED:  %gload47.i._gvload_legalized_rdr = call <1 x double> @llvm.genx.rdregionf.v1f64.v1f64.i16(<1 x double> %gload47.i, i32 0, i32 1, i32 1, i16 0, i32 undef)
; FIXED:  call void @llvm.genx.vstore.v1f64.p0v1f64(<1 x double> zeroinitializer, <1 x double>* @left2)
; FIXED:  %rdr39.i = call <1 x double> @llvm.genx.rdregionf.v1f64.v1f64.i16(<1 x double> %gload47.i._gvload_legalized_rdr, i32 0, i32 1, i32 0, i16 0, i32 0)

; UNFIXED: define spir_kernel void @test()
; UNFIXED:  %gload47.i = call <1 x double> @llvm.genx.vload.v1f64.p0v1f64(<1 x double>* @left2)
; UNFIXED:  call void @llvm.genx.vstore.v1f64.p0v1f64(<1 x double> zeroinitializer, <1 x double>* @left2)
; UNFIXED:  %rdr39.i = call <1 x double> @llvm.genx.rdregionf.v1f64.v1f64.i16(<1 x double> %gload47.i, i32 0, i32 1, i32 0, i16 0, i32 0)

attributes #0 = { "genx_volatile" }
attributes #1 = { "CMGenxMain" }
