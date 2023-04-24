;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXExtractVectorizer -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXExtractVectorizer
; ------------------------------------------------
; This test checks that GenXExtractVectorizer pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_extvec{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
;
; Note: 4 values are optimized out and not salvageble
;
; CHECK-DAG: [[VAL6_V:%[A-z0-9.]*]] = call i16 {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK-DAG: [[VAL7_V:%[A-z0-9.]*]] = call i16 {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK-DAG: [[VAL8_V:%[A-z0-9.]*]] = call i16 {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; CHECK-DAG: [[VAL9_V:%[A-z0-9.]*]] = call i16 {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
; CHECK-DAG: [[VAL10_V:%[A-z0-9.]*]] = add i16 {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]

define void @test_extvec(<4 x i32>* %a, i32* %b) !dbg !6 {
entry:
  %0 = load <4 x i32>, <4 x i32>* %a, !dbg !25
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !25
  %1 = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %0, i32 0, i32 1, i32 1, i16 0, i32 undef), !dbg !26
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !26
  %2 = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %0, i32 0, i32 1, i32 1, i16 4, i32 undef), !dbg !27
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !27
  %3 = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %0, i32 0, i32 1, i32 1, i16 8, i32 undef), !dbg !28
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !28
  %4 = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %0, i32 0, i32 1, i32 1, i16 12, i32 undef), !dbg !29
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !29
  %5 = trunc i32 %1 to i16, !dbg !30
  call void @llvm.dbg.value(metadata i16 %5, metadata !16, metadata !DIExpression()), !dbg !30
  %6 = trunc i32 %2 to i16, !dbg !31
  call void @llvm.dbg.value(metadata i16 %6, metadata !18, metadata !DIExpression()), !dbg !31
  %7 = trunc i32 %3 to i16, !dbg !32
  call void @llvm.dbg.value(metadata i16 %7, metadata !19, metadata !DIExpression()), !dbg !32
  %8 = trunc i32 %4 to i16, !dbg !33
  call void @llvm.dbg.value(metadata i16 %8, metadata !20, metadata !DIExpression()), !dbg !33
  %9 = add i16 %5, %6, !dbg !34
  call void @llvm.dbg.value(metadata i16 %9, metadata !21, metadata !DIExpression()), !dbg !34
  %10 = add i16 %7, %8, !dbg !35
  call void @llvm.dbg.value(metadata i16 %10, metadata !22, metadata !DIExpression()), !dbg !35
  %11 = add i16 %9, %10, !dbg !36
  call void @llvm.dbg.value(metadata i16 %11, metadata !23, metadata !DIExpression()), !dbg !36
  %12 = sext i16 %11 to i32, !dbg !37
  call void @llvm.dbg.value(metadata i32 %12, metadata !24, metadata !DIExpression()), !dbg !37
  store i32 %12, i32* %b, !dbg !38
  ret void, !dbg !39
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "cast.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_extvec", linkageName: "test_extvec", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

declare i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "cast.ll", directory: "/")
!2 = !{}
!3 = !{i32 15}
!4 = !{i32 13}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_extvec", linkageName: "test_extvec", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !18, !19, !20, !21, !22, !23, !24}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !17)
!17 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !17)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !17)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !17)
!21 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !17)
!22 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !17)
!23 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !17)
!24 = !DILocalVariable(name: "13", scope: !6, file: !1, line: 13, type: !12)
!25 = !DILocation(line: 1, column: 1, scope: !6)
!26 = !DILocation(line: 2, column: 1, scope: !6)
!27 = !DILocation(line: 3, column: 1, scope: !6)
!28 = !DILocation(line: 4, column: 1, scope: !6)
!29 = !DILocation(line: 5, column: 1, scope: !6)
!30 = !DILocation(line: 6, column: 1, scope: !6)
!31 = !DILocation(line: 7, column: 1, scope: !6)
!32 = !DILocation(line: 8, column: 1, scope: !6)
!33 = !DILocation(line: 9, column: 1, scope: !6)
!34 = !DILocation(line: 10, column: 1, scope: !6)
!35 = !DILocation(line: 11, column: 1, scope: !6)
!36 = !DILocation(line: 12, column: 1, scope: !6)
!37 = !DILocation(line: 13, column: 1, scope: !6)
!38 = !DILocation(line: 14, column: 1, scope: !6)
!39 = !DILocation(line: 15, column: 1, scope: !6)
