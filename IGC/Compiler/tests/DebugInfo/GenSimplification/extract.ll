;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-shuffle-simplification -S < %s | FileCheck %s
; ------------------------------------------------
; GenSimplification
; ------------------------------------------------
; This test checks that GenSimplification pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: store i32 [[VAL5_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]

define void @test(i32 %a, i32* %b, i1 %c) !dbg !9 {
  %1 = load i32, i32* %b, !dbg !19
  call void @llvm.dbg.value(metadata i32 %1, metadata !12, metadata !DIExpression()), !dbg !19
  %2 = zext i1 %c to i32, !dbg !20
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !20
  %3 = insertelement <2 x i32> zeroinitializer, i32 %a, i32 0, !dbg !21
  call void @llvm.dbg.value(metadata <2 x i32> %3, metadata !15, metadata !DIExpression()), !dbg !21
  %4 = insertelement <2 x i32> %3, i32 %1, i32 1, !dbg !22
  call void @llvm.dbg.value(metadata <2 x i32> %4, metadata !17, metadata !DIExpression()), !dbg !22
  %5 = extractelement <2 x i32> %4, i32 %2, !dbg !23
  call void @llvm.dbg.value(metadata i32 %5, metadata !18, metadata !DIExpression()), !dbg !23
  store i32 %5, i32* %b, !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "extract.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i32, i32*, i1)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 2}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "extract.ll", directory: "/")
!5 = !{}
!6 = !{i32 7}
!7 = !{i32 5}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !17, !18}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !16)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !13)
!19 = !DILocation(line: 1, column: 1, scope: !9)
!20 = !DILocation(line: 2, column: 1, scope: !9)
!21 = !DILocation(line: 3, column: 1, scope: !9)
!22 = !DILocation(line: 4, column: 1, scope: !9)
!23 = !DILocation(line: 5, column: 1, scope: !9)
!24 = !DILocation(line: 6, column: 1, scope: !9)
!25 = !DILocation(line: 7, column: 1, scope: !9)
