;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: automatically reduced from stack_Distort_l0.
; COM: Main purpose of this test is to check that rdr "%3" won't be removed.

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:32:32-i64:64-n8:16:32"

; CHECK: %[[FAKERET:[^ ]+]] = tail call spir_func i32 @foo
; CHECK-NEXT: %[[RETV:[^ ]+]] = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.i32(i32 9, i32 %[[FAKERET]])
; CHECK-NEXT: %[[REALRET:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v96i32.i16(<96 x i32> %[[RETV]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: bitcast i32 %[[REALRET]] to <4 x i8>

define dllexport spir_kernel void @Bulge() local_unnamed_addr {
  %1 = tail call spir_func i32 @foo(), !FuncRetSize !0, !FuncArgSize !1
  %2 = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.i32(i32 9, i32 %1)
  %3 = call i32 @llvm.genx.rdregioni.i32.v96i32.i16(<96 x i32> %2, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %4 = bitcast i32 %3 to <4 x i8>
  %5 = call i8 @llvm.genx.rdregioni.i8.v4i8.i16(<4 x i8> %4, i32 4, i32 1, i32 4, i16 0, i32 undef)
  %sev.cast.29 = bitcast i8 %5 to <1 x i8>
  %6 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v1i8.i16.i1(<256 x i8> undef, <1 x i8> %sev.cast.29, i32 0, i32 1, i32 0, i16 0, i32 32, i1 true)
  tail call void @llvm.genx.media.st.v256i8(i32 0, i32 1, i32 0, i32 24, i32 undef, i32 undef, <256 x i8> %6)
  ret void
}

declare <96 x i32> @llvm.genx.read.predef.reg.v96i32.i32(i32, i32)

declare i32 @llvm.genx.rdregioni.i32.v96i32.i16(<96 x i32>, i32, i32, i32, i16, i32)

declare i8 @llvm.genx.rdregioni.i8.v4i8.i16(<4 x i8>, i32, i32, i32, i16, i32)

declare spir_func i32 @foo()

declare void @llvm.genx.media.st.v256i8(i32, i32, i32, i32, i32, i32, <256 x i8>)

declare <256 x i8> @llvm.genx.wrregioni.v256i8.v1i8.i16.i1(<256 x i8>, <1 x i8>, i32, i32, i32, i16, i32, i1) #0

attributes #0 = { nounwind }

!0 = !{i32 1}
!1 = !{i32 32}
