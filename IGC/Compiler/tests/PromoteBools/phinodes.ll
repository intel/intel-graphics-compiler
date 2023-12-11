;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_func void @phinodes() {
in1:
  %0 = alloca i1, align 1
  %1 = load i1, i1* %0
  br label %cond

in2:
  %2 = add i1 0, 0
  br label %cond

in3:
  %3 = add i1 1, 1
  br label %cond

cond:
  %4 = phi i1 [ %1, %in1 ], [ %2, %in2 ], [ 0, %in3]
  %5 = phi i1 [ 0, %in1], [ %2, %in2 ], [ %3, %in3 ]
  br i1 %4, label %out1, label %out2

out1:
  br label %end

out2:
  br label %end

end:
  ret void
}

; CHECK-LABEL:  define spir_func void @phinodes()
; CHECK:        %7 = phi i8 [ %1, %in1 ], [ %4, %in2 ], [ 0, %in3 ]
; CHECK:        %8 = phi i8 [ 0, %in1 ], [ %3, %in2 ], [ %6, %in3 ]
; CHECK:        %9 = trunc i8 %7 to i1
; CHECK:        br i1 %9, label %out1, label %out2
