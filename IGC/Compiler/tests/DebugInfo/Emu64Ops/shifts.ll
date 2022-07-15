;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-emu64ops -S < %s | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------
; This test checks that Emu64Ops pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_emu64{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: = {{.*}}, !dbg [[EXTR1_LOC:![0-9]*]]
; Value is undef here, check DIExpr before filing
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[EXTR1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR1_LOC]]
; CHECK: = {{.*}}, !dbg [[EXTR2_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[EXTR2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR2_LOC]]
; CHECK: = {{.*}}, !dbg [[LSHL_LOC:![0-9]*]]
; CHECK: phi {{.*}} [[LSHL_LOC]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[LSHL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LSHL_LOC]]
; CHECK: = {{.*}}, !dbg [[LSHR_LOC:![0-9]*]]
; CHECK: phi {{.*}} [[LSHR_LOC]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[LSHR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LSHR_LOC]]
; CHECK: = {{.*}}, !dbg [[ASHR_LOC:![0-9]*]]
; CHECK: phi {{.*}} [[ASHR_LOC]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[ASHR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ASHR_LOC]]
; CHECK-DAG: llvm.dbg.value(metadata <2 x i64> [[INS_V:%[0-9A-z]*]], metadata [[INS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INS_LOC:![0-9]*]]
; CHECK-DAG: [[INS_V]] = {{.*}} [[INS_LOC]]

define void @test_emu64(<2 x i64> %a, <2 x i64>* %dst) !dbg !9 {
  %1 = extractelement <2 x i64> %a, i32 0, !dbg !20
  call void @llvm.dbg.value(metadata i64 %1, metadata !12, metadata !DIExpression()), !dbg !20
  %2 = extractelement <2 x i64> %a, i32 1, !dbg !21
  call void @llvm.dbg.value(metadata i64 %2, metadata !14, metadata !DIExpression()), !dbg !21
  %3 = shl i64 %2, %1, !dbg !22
  call void @llvm.dbg.value(metadata i64 %3, metadata !15, metadata !DIExpression()), !dbg !22
  %4 = lshr i64 %3, %2, !dbg !23
  call void @llvm.dbg.value(metadata i64 %4, metadata !16, metadata !DIExpression()), !dbg !23
  %5 = ashr i64 %4, %3, !dbg !24
  call void @llvm.dbg.value(metadata i64 %5, metadata !17, metadata !DIExpression()), !dbg !24
  %6 = insertelement <2 x i64> %a, i64 %5, i32 0, !dbg !25
  call void @llvm.dbg.value(metadata <2 x i64> %6, metadata !18, metadata !DIExpression()), !dbg !25
  store <2 x i64> %6, <2 x i64>* %dst, align 4, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "shifts.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_emu64", linkageName: "test_emu64", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[EXTR1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[EXTR1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[EXTR2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LSHL_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LSHL_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LSHR_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[LSHR_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ASHR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[ASHR_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[INS_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[INS_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (<2 x i64>, <2 x i64>*)* @test_emu64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "shifts.ll", directory: "/")
!5 = !{}
!6 = !{i32 8}
!7 = !{i32 6}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_emu64", linkageName: "test_emu64", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16, !17, !18}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !13)
!17 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !13)
!18 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 6, type: !19)
!19 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!20 = !DILocation(line: 1, column: 1, scope: !9)
!21 = !DILocation(line: 2, column: 1, scope: !9)
!22 = !DILocation(line: 3, column: 1, scope: !9)
!23 = !DILocation(line: 4, column: 1, scope: !9)
!24 = !DILocation(line: 5, column: 1, scope: !9)
!25 = !DILocation(line: 6, column: 1, scope: !9)
!26 = !DILocation(line: 7, column: 1, scope: !9)
!27 = !DILocation(line: 8, column: 1, scope: !9)
