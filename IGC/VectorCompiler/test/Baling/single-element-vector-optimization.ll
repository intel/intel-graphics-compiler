;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks that prologue optimizations in GenXFuncBaling pass does not produce GEPs when
; folding bitcasts in global store/load instructions operating with SEVs.

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-NOT: getelementptr

@sevgv = internal global <1 x i32> undef, align 4 #0


define dllexport spir_kernel void @test() local_unnamed_addr #1 {
entry:
  %gload.i = load volatile <1 x i32>, <1 x i32>* @sevgv
  %sev.cast.1 = bitcast <1 x i32> %gload.i to i32
  %add.i = add i32 %sev.cast.1, 1
  %sev.cast.2 = bitcast i32 %add.i to <1 x i32>
  store volatile <1 x i32> %sev.cast.2, <1 x i32>* @sevgv
  ret void
}

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCSingleElementVector"="0" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { noinline nounwind "CMGenxMain" }

