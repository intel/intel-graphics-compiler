;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLegalization
; ------------------------------------------------
; This test checks that GenXLegalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: <4 x i32> @test_transform{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <16 x i8> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value({{.*}}, metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i8> [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]

define <4 x i32> @test_transform(<4 x i32>* %a) !dbg !6 {
entry:
  %0 = load <4 x i32>, <4 x i32>* %a, !dbg !16
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = bitcast <4 x i32> %0 to <16 x i8>, !dbg !17
  call void @llvm.dbg.value(metadata <16 x i8> %1, metadata !11, metadata !DIExpression()), !dbg !17
  %2 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v16i8.i16(<16 x i8> %1, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !18
  call void @llvm.dbg.value(metadata <16 x i8> %2, metadata !12, metadata !DIExpression()), !dbg !18
  %3 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v16i8.i8.i1(<16 x i8> %1, <16 x i8> %2, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true), !dbg !19
  call void @llvm.dbg.value(metadata <16 x i8> %3, metadata !13, metadata !DIExpression()), !dbg !19
  %4 = bitcast <16 x i8> %3 to <4 x i32>, !dbg !20
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !14, metadata !DIExpression()), !dbg !20
  %5 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16.i1(<4 x i32> %0, <4 x i32> %4, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true), !dbg !21
  call void @llvm.dbg.value(metadata <4 x i32> %5, metadata !15, metadata !DIExpression()), !dbg !21
  ret <4 x i32> %5, !dbg !22
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "transform-move.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_transform", linkageName: "test_transform", scope: null, file: [[FILE]], line: 1
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

declare <16 x i8> @llvm.genx.rdregioni.v16i8.v16i8.i16(<16 x i8>, i32, i32, i32, i16, i32)

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)

declare <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v16i8.i8.i1(<16 x i8>, <16 x i8>, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "transform-move.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_transform", linkageName: "test_transform", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
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
