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
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V:%[A-z0-9.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL3_V:%[A-z0-9.]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; Note: VAL2 loc is ok, but id went with VAL3
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: merge:
; rdregion and select are opted out
; CHECK-DAG: store <4 x i32> [[VAL6_V:%[A-z0-9.]*]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]

define void @test_patternmatch(<4 x i32>* %a, <4 x i1> %b) !dbg !6 {
entry:
  %0 = load <4 x i32>, <4 x i32>* %a, !dbg !16
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = select <4 x i1> %b, <4 x i32> %0, <4 x i32> zeroinitializer, !dbg !17
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !17
  %2 = sub <4 x i32> %0, %1, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !12, metadata !DIExpression()), !dbg !18
  store <4 x i32> %2, <4 x i32>* %a, !dbg !19
  br label %merge, !dbg !20

merge:                                            ; preds = %entry
  %3 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %0, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !21
  call void @llvm.dbg.value(metadata <4 x i32> %3, metadata !13, metadata !DIExpression()), !dbg !21
  %4 = select <4 x i1> %b, <4 x i32> %3, <4 x i32> zeroinitializer, !dbg !22
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !14, metadata !DIExpression()), !dbg !22
  %5 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16.i1(<4 x i32> %0, <4 x i32> %4, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true), !dbg !23
  call void @llvm.dbg.value(metadata <4 x i32> %5, metadata !15, metadata !DIExpression()), !dbg !23
  store <4 x i32> %5, <4 x i32>* %a, !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "simplify-select.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "simplify-select.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 6, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !10)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
!24 = !DILocation(line: 9, column: 1, scope: !6)
!25 = !DILocation(line: 10, column: 1, scope: !6)
