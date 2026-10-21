;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Late InstCombine in the codegen pipeline may fold the trunc/zext pair below
; into "and (freeze %gather), <-1, -1, 0, 0>". The freeze must be removed
; before vISA emission, which cannot lower it.

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeLP -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeLP -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

target triple = "genx64-unknown-unknown"

declare <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(<1 x i1>, i32, <1 x i64>, <4 x i8>)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v4i8(<1 x i1>, i32, <1 x i64>, <4 x i8>)

; CHECK: .kernel "test"
; CHECK: svm_gather
; CHECK: svm_scatter

define spir_kernel void @test(i64 %src, i64 %dst) #0 {
  %src.v = insertelement <1 x i64> poison, i64 %src, i64 0
  %gather = call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(<1 x i1> <i1 true>, i32 1, <1 x i64> %src.v, <4 x i8> undef)
  %cast = bitcast <4 x i8> %gather to <1 x i32>
  %trunc = trunc <1 x i32> %cast to <1 x i16>
  %scalar = bitcast <1 x i16> %trunc to i16
  %vec = insertelement <1 x i16> poison, i16 %scalar, i64 0
  %zext = zext <1 x i16> %vec to <1 x i32>
  %res = bitcast <1 x i32> %zext to <4 x i8>
  %dst.v = insertelement <1 x i64> poison, i64 %dst, i64 0
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v4i8(<1 x i1> <i1 true>, i32 1, <1 x i64> %dst.v, <4 x i8> %res)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i64, i64)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0}
!2 = !{i32 64, i32 72}
!3 = !{i32 0, i32 0}
!4 = !{!"", !""}
!5 = !{void (i64, i64)* @test, null, null, null, null}
