;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -S < %s | FileCheck %s

declare <16 x i8> @some.intrinsic()
declare void @another.intrinsic(<16 x i32>)
declare <16 x i32>  @llvm.genx.smin.v16i32.v16i32(<16 x i32>, <16 x i32>)

;CHECK:  %[[REDUCE:[^ ]+]] = sext <16 x i8> %2 to <16 x i16>
;CHECK:  [[CALL:[^ ]+]] = call <16 x i32> @llvm.genx.smin.v16i32.v16i16(<16 x i16> %[[REDUCE]], <16 x i16> <i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180, i16 -180>)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test(i32 %0) {
  %2 = call <16 x i8> @some.intrinsic()
  %3 = sext <16 x i8> %2 to <16 x i32>
  %4 = call <16 x i32> @llvm.genx.smin.v16i32.v16i32(<16 x i32> %3, <16 x i32> <i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180, i32 -180>)
  call void @another.intrinsic(<16 x i32> %4)
  ret void
}

