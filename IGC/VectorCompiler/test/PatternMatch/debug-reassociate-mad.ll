;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPatternMatch
; ------------------------------------------------
; This test checks that GenXPatternMatch pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: void @test_patternmatch{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: [[VAL8_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: [[VAL4T_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4T_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]

define void @test_patternmatch(i32* %a) !dbg !6 {
  %1 = load i32, i32* %a, !dbg !18
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !18
  %2 = mul i32 %1, 13, !dbg !19
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !19
  %3 = shl i32 %1, 4, !dbg !20
  call void @llvm.dbg.value(metadata i32 %3, metadata !12, metadata !DIExpression()), !dbg !20
  %4 = add i32 %2, %3, !dbg !21
  call void @llvm.dbg.value(metadata i32 %4, metadata !13, metadata !DIExpression()), !dbg !21
  %5 = mul i32 %1, 15, !dbg !22
  call void @llvm.dbg.value(metadata i32 %5, metadata !14, metadata !DIExpression()), !dbg !22
  %6 = mul i32 %1, %1, !dbg !23
  call void @llvm.dbg.value(metadata i32 %6, metadata !15, metadata !DIExpression()), !dbg !23
  %7 = add i32 %5, %6, !dbg !24
  call void @llvm.dbg.value(metadata i32 %7, metadata !16, metadata !DIExpression()), !dbg !24
  %8 = add i32 %4, %7, !dbg !25
  call void @llvm.dbg.value(metadata i32 %8, metadata !17, metadata !DIExpression()), !dbg !25
  store i32 %8, i32* %a, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "reassociate-mad.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "reassociate-mad.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16, !17}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
!25 = !DILocation(line: 8, column: 1, scope: !6)
!26 = !DILocation(line: 9, column: 1, scope: !6)
!27 = !DILocation(line: 10, column: 1, scope: !6)
