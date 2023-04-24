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
; CHECK: void @llvm.dbg.value(metadata <16 x i16> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; its debatable what line info should be preserved here
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; Note: for llvm-14 this value can be saved.
; CHECK: void @llvm.dbg.value({{.*}}, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression({{.*}})), !dbg [[VAL2_LOC]]
; CHECK: void @llvm.dbg.value(metadata <16 x i1> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i16 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]


define void @test_patternmatch(<16 x i16>* %a, i16* %b) !dbg !6 {
  %1 = load <16 x i16>, <16 x i16>* %a, !dbg !16
  call void @llvm.dbg.value(metadata <16 x i16> %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = icmp eq <16 x i16> %1, zeroinitializer, !dbg !17
  call void @llvm.dbg.value(metadata <16 x i1> %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = xor <16 x i1> %2, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, !dbg !18
  call void @llvm.dbg.value(metadata <16 x i1> %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = bitcast <16 x i1> %3 to i16, !dbg !19
  call void @llvm.dbg.value(metadata i16 %4, metadata !14, metadata !DIExpression()), !dbg !19
  store i16 %4, i16* %b, !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "not.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "not.ll", directory: "/")
!2 = !{}
!3 = !{i32 6}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
!15 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
