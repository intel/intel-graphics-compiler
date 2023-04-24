;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXUnbalingWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXUnbaling
; ------------------------------------------------
; This test checks that GenXUnbaling pass follows
; 'How to Update Debug Info' llvm guideline.

; CHECK: define void @test_unbale{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]

define void @test_unbale(<16 x i32>* %a, <16 x i32>* %b) #0 !dbg !6 {
entry:
  %load = load <16 x i32>, <16 x i32>* %a, !dbg !16
  call void @llvm.dbg.value(metadata <16 x i32> %load, metadata !9, metadata !DIExpression()), !dbg !16
  %load1 = load <16 x i32>, <16 x i32>* %b, !dbg !17
  call void @llvm.dbg.value(metadata <16 x i32> %load1, metadata !11, metadata !DIExpression()), !dbg !17
  %rdregioni = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> %load1, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !18
  call void @llvm.dbg.value(metadata <16 x i32> %rdregioni, metadata !12, metadata !DIExpression()), !dbg !18
  %wrregioni = call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i32.i1(<16 x i32> %rdregioni, <16 x i32> %load, i32 1, i32 1, i32 0, i16 4, i32 0, i1 true), !dbg !19
  call void @llvm.dbg.value(metadata <16 x i32> %wrregioni, metadata !13, metadata !DIExpression()), !dbg !19
  %wrregioni2 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i32.i1(<16 x i32> %wrregioni, <16 x i32> %load, i32 1, i32 1, i32 0, i16 6, i32 0, i1 true), !dbg !20
  call void @llvm.dbg.value(metadata <16 x i32> %wrregioni2, metadata !14, metadata !DIExpression()), !dbg !20
  %rdregioni3 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> %wrregioni, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !21
  call void @llvm.dbg.value(metadata <16 x i32> %rdregioni3, metadata !15, metadata !DIExpression()), !dbg !21
  store <16 x i32> %rdregioni3, <16 x i32>* %b, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "shorten-lr.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_unbale", linkageName: "test_unbale", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32)

declare <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i32.i1(<16 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "shorten-lr.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_unbale", linkageName: "test_unbale", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
