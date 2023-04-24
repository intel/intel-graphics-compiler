;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: check that GenXCoalescing does not fail on SIMD
; COM: test is expected just to run compilation without extra checks
; RUN: llc %s -march=genx64 -mcpu=Gen9  -mattr=+ocl_runtime \
; RUN: -vc-disable-coalescing \
; RUN: -o /dev/null

; ModuleID = 'start.ll'
source_filename = "start.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@EM = internal global <32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>

declare { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1>, <16 x i1>, <16 x i1>)

declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1>, <16 x i1>)

define dllexport spir_kernel void @"simd<unsigned int, unsigned int>"(i32 %0, i32 %1, i32 %2, i16 %3, i64 %privBase) #0 {
  %EM.local = alloca <32 x i1>, align 32
  store <32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1>* %EM.local, align 32
  %5 = load <32 x i1>, <32 x i1>* %EM.local, align 32
  %6 = call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1> %5, <16 x i1> zeroinitializer, <16 x i1> undef)
  %7 = extractvalue { <32 x i1>, <16 x i1>, i1 } %6, 0
  store <32 x i1> %7, <32 x i1>* %EM.local, align 32
  %8 = extractvalue { <32 x i1>, <16 x i1>, i1 } %6, 1
  %9 = extractvalue { <32 x i1>, <16 x i1>, i1 } %6, 2
  br i1 %9, label %17, label %10

10:                                               ; preds = %4
  %11 = call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1> %7, <16 x i1> zeroinitializer, <16 x i1> undef)
  %12 = extractvalue { <32 x i1>, <16 x i1>, i1 } %11, 0
  %13 = extractvalue { <32 x i1>, <16 x i1>, i1 } %11, 1
  br label %14

14:                                               ; preds = %10
  %15 = call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1> %12, <16 x i1> %13)
  %16 = extractvalue { <32 x i1>, i1 } %15, 0
  store <32 x i1> %16, <32 x i1>* %EM.local, align 32
  br label %17

17:                                               ; preds = %14, %4
  %18 = load <32 x i1>, <32 x i1>* %EM.local, align 32
  %19 = call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1> %18, <16 x i1> %8)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i32, i32, i16, i64)* @"simd<unsigned int, unsigned int>", !"simd<unsigned int, unsigned int>", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2, i32 2, i32 2, i32 0, i32 96}
!2 = !{i32 -1, i32 -1, i32 -1, i32 72, i32 64}
!3 = !{i32 0, i32 0, i32 0, i32 0}
!4 = !{!"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !""}
!5 = !{void (i32, i32, i32, i16, i64)* @"simd<unsigned int, unsigned int>", !6, !7, !8, !9}
!6 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3, i32 4}
!8 = !{}
!9 = !{i32 0, i32 1, i32 2, i32 -1, i32 3}
