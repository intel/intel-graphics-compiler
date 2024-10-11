;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %opt %use_old_pass_manager% -GenXFloatControl -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | \
; RUN: FileCheck %s --check-prefix=FLUSH
; RUN: %opt %use_old_pass_manager% -GenXFloatControl -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=Xe2 -S < %s | \
; RUN: FileCheck %s --check-prefix=RETAIN

; FLUSH: define dllexport spir_kernel void @the_test(i32 %0, i32 %1)
; FLUSH-NOT: predef
; FLUSH-NEXT: ret void

; RETAIN: define dllexport spir_kernel void @the_test(i32 %0, i32 %1)
; RETAIN-NEXT: [[AND_READ_PREDEF:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; RETAIN-NEXT: [[AND_RDREGION:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[AND_READ_PREDEF]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; RETAIN-NEXT: [[AND:[^ ]+]] = and i32 [[AND_RDREGION]], -1073743089
; RETAIN-NEXT: [[AND_WRREGION:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[AND_READ_PREDEF]], i32 [[AND]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; RETAIN-NEXT: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[AND_WRREGION]])
; RETAIN-NEXT: [[OR_READ_PREDEF:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; RETAIN-NEXT: [[OR_RDREGION:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[OR_READ_PREDEF]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; RETAIN-NEXT: [[OR:[^ ]+]] = or i32 [[OR_RDREGION]], 1073741824
; RETAIN-NEXT: [[OR_WRREGION:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[OR_READ_PREDEF]], i32 [[OR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; RETAIN-NEXT: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[OR_WRREGION]])
; RETAIN-NEXT: ret void

define dllexport spir_kernel void @the_test(i32 %0, i32 %1) #0 {
  ret void
}

attributes #0 = { "CMGenxMain" "CMFloatControl"="0" }
