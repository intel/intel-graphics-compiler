;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-type-legalizer -S < %s | FileCheck %s

; Test checks phi promotion

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @test_phi(i1 %src) {
; CHECK-LABEL: @test_phi(
; CHECK:    [[DOTPROMOTE:%.*]] = phi i8 [ -2, %b2 ], [ 12, %b1 ]
; CHECK:    call void @use.i8(i8 [[DOTPROMOTE]])
; CHECK:    ret void
;
  br i1 %src, label %b1, label %b2
b1:
  %1 = add i4 5, 7
  br label %end
b2:
  %2 = sub i4 6, 8
  br label %end
end:
  %3 = phi i4 [%2, %b2], [%1, %b1]
  %4 = sext i4 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

declare void @use.i8(i8)
