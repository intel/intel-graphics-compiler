;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

@str = internal unnamed_addr constant [5 x i8] c"text\00", align 1 #0
; CHECK-DAG: @str = internal constant [5 x i8] c"text\00", align 1 #[[ORIG_ATTR:[0-9]+]]
; CHECK-DAG: @str.indexed = internal constant [5 x i8] c"text\00", align 1 #[[CLONE_ATTR:[0-9]+]]

declare i32 @llvm.vc.internal.print.format.index.p0i8(i8*)

define dllexport void @str_vs_ptr(<1 x i64> %addr) {
  %legal.use = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str, i64 0, i64 0))
  %illegal.use = ptrtoint [5 x i8]* @str to i64
  ret void
}

attributes #0 = { "some-attr-0" "some-attr-1" "VCPrintfStringVariable" "some-attr-2" }
; CHECK-DAG: attributes #[[ORIG_ATTR]] = { "some-attr-0" "some-attr-1" "some-attr-2" }
; CHECK-DAG: attributes #[[CLONE_ATTR]] = { "VCPrintfStringVariable" }
