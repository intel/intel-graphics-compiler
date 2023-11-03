;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-memopt | FileCheck %s --check-prefix=%LLVM_11_CHECK_PREFIX%

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @func(i8* %dst, i8* %src) {
entry:
  %arrayidx1 = getelementptr inbounds i8, i8* %src, i64 1
  %0 = load i8, i8* %arrayidx1
  %arrayidx2 = getelementptr inbounds i8, i8* %src, i64 2
  %1 = load i8, i8* %arrayidx2, align 2
  %arrayidx3 = getelementptr inbounds i8, i8* %src, i64 3
  %2 = load i8, i8* %arrayidx3
  %arrayidx4 = getelementptr inbounds i8, i8* %src, i64 4
  %3 = load i8, i8* %arrayidx4, align 4
  %arrayidx5 = getelementptr inbounds i8, i8* %src, i64 5
  %4 = load i8, i8* %arrayidx5
  %arrayidx6 = getelementptr inbounds i8, i8* %dst, i64 1
  store i8 %0, i8* %arrayidx6
  %arrayidx7 = getelementptr inbounds i8, i8* %dst, i64 2
  store i8 %1, i8* %arrayidx7, align 2
  %arrayidx8 = getelementptr inbounds i8, i8* %dst, i64 3
  store i8 %2, i8* %arrayidx8
  %arrayidx9 = getelementptr inbounds i8, i8* %dst, i64 4
  store i8 %3, i8* %arrayidx9, align 4
  %arrayidx10 = getelementptr inbounds i8, i8* %dst, i64 5
  store i8 %4, i8* %arrayidx10
  ret void
}

; CHECK-PRE-LLVM-11: MemOpt expects alignment to be always explicitly set for the leading instruction!
; CHECK-PRE-LLVM-11: MemOpt expects alignment to be always explicitly set for the leading instruction!

; LLVM 11 API always sets the alignment. So even if it's not set in this test input, LLVM API assigns the
; alignment based on the instruction type while parsing, therefore expecting the issue to not reproduce for LLVM11+.
; CHECK-LLVM-11-PLUS-NOT: MemOpt expects alignment to be always explicitly set for the leading instruction!
; CHECK-LLVM-11-PLUS-NOT: MemOpt expects alignment to be always explicitly set for the leading instruction!

!igc.functions = !{!0}

!0 = !{void (i8*, i8*)* @func, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
