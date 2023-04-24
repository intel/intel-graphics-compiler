;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXDepressurizer
; ------------------------------------------------
; This test checks that GenXDepressurizer pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define void @test_depressure{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i1> [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]

define void @test_depressure(<16 x i1>* %a, <16 x i1>* %b) #0 !dbg !7 {
entry:
  %0 = load <16 x i1>, <16 x i1>* %a, !dbg !18
  call void @llvm.dbg.value(metadata <16 x i1> %0, metadata !10, metadata !DIExpression()), !dbg !18
  %1 = load <16 x i1>, <16 x i1>* %b, !dbg !19
  call void @llvm.dbg.value(metadata <16 x i1> %1, metadata !12, metadata !DIExpression()), !dbg !19
  %2 = and <16 x i1> %0, %1, !dbg !20
  call void @llvm.dbg.value(metadata <16 x i1> %2, metadata !13, metadata !DIExpression()), !dbg !20
  %3 = icmp eq <16 x i1> %2, %1, !dbg !21
  call void @llvm.dbg.value(metadata <16 x i1> %3, metadata !14, metadata !DIExpression()), !dbg !21
  %4 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v16i1(<16 x i1> %2, <16 x i1> %3, i32 16), !dbg !22
  call void @llvm.dbg.value(metadata <16 x i1> %4, metadata !15, metadata !DIExpression()), !dbg !22
  %5 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v16i1(<16 x i1> %2, <16 x i1> %3, i32 16), !dbg !23
  call void @llvm.dbg.value(metadata <16 x i1> %5, metadata !16, metadata !DIExpression()), !dbg !23
  %6 = and <16 x i1> %5, %3, !dbg !24
  call void @llvm.dbg.value(metadata <16 x i1> %6, metadata !17, metadata !DIExpression()), !dbg !24
  store <16 x i1> %6, <16 x i1>* %b, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "order-num.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_depressure", linkageName: "test_depressure", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

declare <16 x i1> @llvm.genx.wrpredregion.v16i1.v16i1(<16 x i1>, <16 x i1>, i32)

declare <16 x i1> @llvm.genx.rdpredregion.v16i1.v16i1(<16 x i1>, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!llvm.debugify = !{!0, !1, !2, !1}
!llvm.module.flags = !{!3}
!llvm.dbg.cu = !{!4}

!0 = !{i32 10}
!1 = !{i32 7}
!2 = !{i32 9}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "order-num.ll", directory: "/")
!6 = !{}
!7 = distinct !DISubprogram(name: "test_depressure", linkageName: "test_depressure", scope: null, file: !5, line: 1, type: !8, scopeLine: 1, unit: !4, retainedNodes: !9)
!8 = !DISubroutineType(types: !6)
!9 = !{!10, !12, !13, !14, !15, !16, !17}
!10 = !DILocalVariable(name: "1", scope: !7, file: !5, line: 1, type: !11)
!11 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !5, line: 2, type: !11)
!13 = !DILocalVariable(name: "3", scope: !7, file: !5, line: 3, type: !11)
!14 = !DILocalVariable(name: "4", scope: !7, file: !5, line: 4, type: !11)
!15 = !DILocalVariable(name: "5", scope: !7, file: !5, line: 5, type: !11)
!16 = !DILocalVariable(name: "6", scope: !7, file: !5, line: 6, type: !11)
!17 = !DILocalVariable(name: "7", scope: !7, file: !5, line: 7, type: !11)
!18 = !DILocation(line: 1, column: 1, scope: !7)
!19 = !DILocation(line: 2, column: 1, scope: !7)
!20 = !DILocation(line: 3, column: 1, scope: !7)
!21 = !DILocation(line: 4, column: 1, scope: !7)
!22 = !DILocation(line: 5, column: 1, scope: !7)
!23 = !DILocation(line: 6, column: 1, scope: !7)
!24 = !DILocation(line: 7, column: 1, scope: !7)
!25 = !DILocation(line: 8, column: 1, scope: !7)
!26 = !DILocation(line: 9, column: 1, scope: !7)
