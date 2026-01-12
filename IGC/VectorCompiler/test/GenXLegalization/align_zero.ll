;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16>, i32, i32, i32, i16, i32)
declare <64 x i16> @llvm.genx.wrregioni.v64i16.v4i16.i16.i1(<64 x i16>, <4 x i16>, i32, i32, i32, i16, i32, i1)

define <64 x i16> @test(<64 x i16> %buff, <4 x i16> %insert, i1 %cmp.1, i1 %cmp.2, i1 %cmp.3) {
entry:
  br i1 %cmp.1, label %l0, label %end

l0:
  br i1 %cmp.2, label %if, label %else

if:
  %ph.0 = phi i16 [ 0, %l0 ], [ %ph.1, %else]
  br label %else

else:
  %ph.1 = phi i16 [ 0, %l0 ], [ %ph.0, %if ]
  %val = sub i16 %ph.1, 0
  br i1 %cmp.3, label %end, label %if

end:
  %idx = phi i16 [ %val, %else ], [ 0, %entry ]
  ; CHECK-NOT: call <1 x i16> @llvm.genx.rdregioni.v1i16.v4i16.i16
  %wrreg = tail call <64 x i16> @llvm.genx.wrregioni.v64i16.v4i16.i16.i1(<64 x i16> %buff, <4 x i16> %insert, i32 0, i32 2, i32 1, i16 %idx, i32 undef, i1 true)
  %res = add <64 x i16> %wrreg, %buff
  ret <64 x i16> %res
}
