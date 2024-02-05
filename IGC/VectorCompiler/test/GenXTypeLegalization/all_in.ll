;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTypeLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

define spir_func void @foo(i64 %in) {
entry:
; CHECK: [[TRUNC:[^ ]+]] = trunc i64 %in to i16
; CHECK: %src.l = and i16 [[TRUNC]], 2047
; CHECK: %cast0.l = and i16 %src.l, 1023
; CHECK: [[MASKED:[^ ]+]] = and i16 %src.l, 2047
; CHECK: switch i16 [[MASKED]], label %if
; CHECK:   i16 2047, label %if.else
; CHECK:   i16 2046, label %end
  %src = trunc i64 %in to i11
  %cast0 = trunc i11 %src to i10
  switch i11 %src, label %if [
    i11 -1, label %if.else
    i11 -2, label %end
  ]

if:
; CHECK: %cast1.l = and i16 %src.l, 1023
  %cast1 = trunc i11 %src to i10
  br label %end

if.else:
  br label %end

end:
; CHECK: %ph.l = phi i16 [ %cast0.l, %entry ], [ %cast1.l, %if ], [ 1019, %if.else ]
; CHECK: %tr.l = and i16 %ph.l, 511
  %ph = phi i10 [ %cast0, %entry], [ %cast1, %if], [ -5, %if.else]
  %tr = trunc i10 %ph to i9
  ret void
}
