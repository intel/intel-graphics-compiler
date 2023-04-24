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

;CHECK:  %3 = sext <16 x i8> %2 to <16 x i16>
;CHECK:  %4 = and <16 x i16> %3, <i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449, i16 -13449>
;CHECK:  %.cast = zext <16 x i16> %4 to <16 x i32>

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test(i32 %0) {
  %2 = call <16 x i8> @some.intrinsic()
  %3 = sext <16 x i8> %2 to <16 x i32>
  %4 = and <16 x i32> %3, <i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087, i32 52087>
  call void @another.intrinsic(<16 x i32> %4)
  ret void
}

