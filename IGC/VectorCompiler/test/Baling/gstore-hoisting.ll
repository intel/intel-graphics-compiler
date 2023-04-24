;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

@All = internal global <1024 x i32> undef, align 4096

declare <1024 x i32> @llvm.genx.wrregioni.v1024i32.v32i32.i16.i1(<1024 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1)
declare <2048 x i16> @llvm.genx.wrregioni.v2048i16.v64i16.i16.i1(<2048 x i16>, <64 x i16>, i32, i32, i32, i16, i32, i1)
declare i1 @llvm.genx.any.v16i1(<16 x i1>)

; CHECK-LABEL: test
; CHECK: store volatile <2048 x i16> %wrr1, <2048 x i16>* bitcast (<1024 x i32>* @All to <2048 x i16>*){{(, align 4096)?}}: g_store 0
; CHECK: store volatile <1024 x i32> %wrr2, <1024 x i32>* @All{{(, align 4096)?}}: g_store 0

define void @test(<64 x i16> %data1, <32 x i32> %data2, <16 x i1> %mask) {
entry:
  %load1 = load volatile <2048 x i16>, <2048 x i16>* bitcast (<1024 x i32>* @All to <2048 x i16>*)
  %wrr1 = tail call <2048 x i16> @llvm.genx.wrregioni.v2048i16.v64i16.i16.i1(<2048 x i16> %load1, <64 x i16> %data1, i32 0, i32 64, i32 1, i16 0, i32 undef, i1 true)
  br label %header

header:
  %sink = phi <2048 x i16> [ %wrr1, %entry ], [ %wrr2.cast, %loop ]
  %cast = bitcast <2048 x i16> %sink to <1024 x i32>
  store volatile <1024 x i32> %cast, <1024 x i32>* @All
  %cond = call i1 @llvm.genx.any.v16i1(<16 x i1> %mask)
  br i1 %cond, label %loop, label %exit

loop:
  %load2 = load volatile <1024 x i32>, <1024 x i32>* @All
  %wrr2 = tail call <1024 x i32> @llvm.genx.wrregioni.v1024i32.v32i32.i16.i1(<1024 x i32> %load2, <32 x i32> %data2, i32 0, i32 32, i32 1, i16 0, i32 undef, i1 true)
  %wrr2.cast = bitcast <1024 x i32> %wrr2 to <2048 x i16>
  br label %header

exit:
  ret void
}
