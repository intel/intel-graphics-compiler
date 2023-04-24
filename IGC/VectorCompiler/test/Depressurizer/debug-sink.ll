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
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]

define void @test_depressure(<512 x i32> %a, <512 x i32>* %b) #0 !dbg !6 {
entry:
  %0 = lshr <512 x i32> %a, %a, !dbg !17
  call void @llvm.dbg.value(metadata <512 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = lshr <512 x i32> %a, %a, !dbg !18
  call void @llvm.dbg.value(metadata <512 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !18
  %2 = add <512 x i32> %0, %0, !dbg !19
  call void @llvm.dbg.value(metadata <512 x i32> %2, metadata !12, metadata !DIExpression()), !dbg !19
  %3 = add <512 x i32> %1, %1, !dbg !20
  call void @llvm.dbg.value(metadata <512 x i32> %3, metadata !13, metadata !DIExpression()), !dbg !20
  %4 = call <512 x i32> @llvm.genx.wrregioni.v512i32.v512i32.v512i32.i32.i1(<512 x i32> %2, <512 x i32> %3, i32 1, i32 1, i32 0, i16 16, i32 0, i1 true), !dbg !21
  call void @llvm.dbg.value(metadata <512 x i32> %4, metadata !14, metadata !DIExpression()), !dbg !21
  %5 = call <512 x i32> @llvm.genx.wrregioni.v512i32.v512i32.v512i32.i32.i1(<512 x i32> %2, <512 x i32> %3, i32 1, i32 1, i32 0, i16 16, i32 0, i1 true), !dbg !22
  call void @llvm.dbg.value(metadata <512 x i32> %5, metadata !15, metadata !DIExpression()), !dbg !22
  %6 = add <512 x i32> %5, %5, !dbg !23
  call void @llvm.dbg.value(metadata <512 x i32> %6, metadata !16, metadata !DIExpression()), !dbg !23
  store <512 x i32> %5, <512 x i32>* %b, !dbg !24
  store <512 x i32> %6, <512 x i32>* %b, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "sink.ll", directory: "/")
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

declare <512 x i32> @llvm.genx.rdregioni.v512i32.v512i32.i16(<512 x i32>, i32, i32, i32, i16, i32)

declare <512 x i32> @llvm.genx.wrregioni.v512i32.v512i32.v512i32.i32.i1(<512 x i32>, <512 x i32>, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!llvm.debugify = !{!0, !1, !0, !1}
!llvm.module.flags = !{!2}
!llvm.dbg.cu = !{!3}

!0 = !{i32 10}
!1 = !{i32 7}
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "sink.ll", directory: "/")
!5 = !{}
!6 = distinct !DISubprogram(name: "test_depressure", linkageName: "test_depressure", scope: null, file: !4, line: 1, type: !7, scopeLine: 1, unit: !3, retainedNodes: !8)
!7 = !DISubroutineType(types: !5)
!8 = !{!9, !11, !12, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !4, line: 1, type: !10)
!10 = !DIBasicType(name: "ty16384", size: 16384, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !4, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !4, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !4, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !4, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !4, line: 6, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !4, line: 7, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
