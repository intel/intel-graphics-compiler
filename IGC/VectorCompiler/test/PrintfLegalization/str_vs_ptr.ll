;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

@str = internal unnamed_addr constant [5 x i8] c"text\00", align 1 #0
; CHECK: @str = internal constant [5 x i8] c"text\00", align 1
; CHECK-NOT: #0
; CHECK-NEXT: @str.indexed = internal constant [5 x i8] c"text\00", align 1 #0

declare i32 @llvm.vc.internal.print.format.index.p0i8(i8*)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1>, i32, <1 x i64>, <1 x i32>)

define dllexport void @str_vs_ptr(<1 x i64> %addr) {
  %gep = getelementptr inbounds [5 x i8], [5 x i8]* @str, i64 0, i64 0
  %idx.gep = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* %gep)
; CHECK: %idx.gep = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.indexed, i32 0, i32 0))
  %idx.direct = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str, i64 0, i64 0))
; CHECK: %idx.direct = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.indexed, i32 0, i32 0))
  tail call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %addr, <1 x i32> <i32 extractelement (<2 x i32> bitcast (i64 ptrtoint ([5 x i8]* @str to i64) to <2 x i32>), i16 0)>)
; COM: non-print.format.index use should stay the same
; CHECK: tail call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %addr, <1 x i32> <i32 extractelement (<2 x i32> bitcast (i64 ptrtoint ([5 x i8]* @str to i64) to <2 x i32>), i16 0)>)
  ret void
}

attributes #0 = { "VCPrintfStringVariable" }
; CHECK: attributes #0 = { "VCPrintfStringVariable" }
