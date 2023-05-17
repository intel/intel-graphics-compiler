;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; COM: Have to rely on GV order here to use CHECK-NOT.
@str.a = internal unnamed_addr constant [2 x i8] c"a\00", align 1 #0
; CHECK: @str.a = internal constant [2 x i8] c"a\00", align 1
; CHECK-NOT: #0
@str.b = internal unnamed_addr constant [2 x i8] c"b\00", align 1 #0
; CHECK-NEXT: @str.b = internal constant [2 x i8] c"b\00", align 1
; CHECK-NOT: #0
; COM: not apply
@str.no.a = internal unnamed_addr constant [5 x i8] c"no.a\00", align 1 #0
; CHECK-NEXT: @str.no.a = internal unnamed_addr constant [5 x i8] c"no.a\00", align 1 #0
@str.no.b = internal unnamed_addr constant [5 x i8] c"no.b\00", align 1 #0
; CHECK-NEXT: @str.no.b = internal unnamed_addr constant [5 x i8] c"no.b\00", align 1 #0

; COM: Cloned strings.
; CHECK-NEXT: @str.a.indexed = internal constant [2 x i8] c"a\00", align 1 #0
; CHECK-NEXT: @str.b.indexed = internal constant [2 x i8] c"b\00", align 1 #0

declare i32 @llvm.vc.internal.print.format.index.p0i8(i8*)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1>, i32, <1 x i64>, <1 x i32>)

define dllexport void @mixed_users() {
  %a.gep.legal = getelementptr inbounds [2 x i8], [2 x i8]* @str.a, i64 0, i64 0
  %idx.a.gep.legal.0 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* %a.gep.legal)
  %idx.a.gep.legal.1 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* %a.gep.legal)
; COM: the same line
; CHECK: %idx.a.gep.legal.0 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.a.indexed, i32 0, i32 0))
; CHECK: %idx.a.gep.legal.1 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.a.indexed, i32 0, i32 0))

  %a.gep.mixed = getelementptr inbounds [2 x i8], [2 x i8]* @str.a, i64 0, i64 0
; COM: GEP is used outside format index thus it must be preserved.
; CHECK: %a.gep.mixed = getelementptr inbounds [2 x i8], [2 x i8]* @str.a, i64 0, i64 0
  %idx.a.gep.mixed = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* %a.gep.mixed)
  %usr.a.gep.mixed = ptrtoint i8* %a.gep.mixed to i64
; CHECK: %idx.a.gep.mixed = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.a.indexed, i32 0, i32 0))
; COM: the same line, continues using the original gep
; CHECK: %usr.a.gep.mixed = ptrtoint i8* %a.gep.mixed to i64


  %idx.b.direct.1 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.b, i64 0, i64 0))
  %idx.b.direct.2 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.b, i64 0, i64 0))
; CHECK: %idx.b.direct.1 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.b.indexed, i32 0, i32 0))
; CHECK: %idx.b.direct.2 = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.b.indexed, i32 0, i32 0))
  %usr.b.direct = ptrtoint i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.b, i64 0, i64 0) to i64
; CHECK: %usr.b.direct = ptrtoint i8* getelementptr inbounds ([2 x i8], [2 x i8]* @str.b, i64 0, i64 0) to i64


; COM: originally legal indexed string case
  %idx.no.a.direct = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.no.a, i64 0, i64 0))
  %no.a.gep.legal = getelementptr inbounds [5 x i8], [5 x i8]* @str.no.a, i64 0, i64 0
  %idx.no.a.gep.legal = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* %no.a.gep.legal)
; COM: no transformation is applied, the lines stay the same
; CHECK: %idx.no.a.direct = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.no.a, i64 0, i64 0))
; CHECK: %no.a.gep.legal = getelementptr inbounds [5 x i8], [5 x i8]* @str.no.a, i64 0, i64 0
; CHECK: %idx.no.a.gep.legal = tail call i32 @llvm.vc.internal.print.format.index.p0i8(i8* %no.a.gep.legal)

; COM: not indexed string at all
  %usr.no.b.direct = ptrtoint i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.no.b, i64 0, i64 0) to i64
  %usr.no.b = ptrtoint [5 x i8]* @str.no.b to i64
; CHECK: %usr.no.b.direct = ptrtoint i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.no.b, i64 0, i64 0) to i64
; CHECK: %usr.no.b = ptrtoint [5 x i8]* @str.no.b to i64

  ret void
}

attributes #0 = { "VCPrintfStringVariable" }
; CHECK: attributes #0 = { "VCPrintfStringVariable" }
