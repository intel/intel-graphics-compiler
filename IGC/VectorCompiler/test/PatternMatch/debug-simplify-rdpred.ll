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
; CHECK: void @llvm.dbg.value(metadata i8 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
;
; new bitcast an its use, rdpred dce
; CHECK-DAG: void @llvm.dbg.value(metadata <8 x i1> [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK: void @llvm.dbg.value(metadata i8 %{{[A-z0-9.]*}}, metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]

define void @test_patternmatch(i8* %a) !dbg !6 {
entry:
  %0 = load i8, i8* %a, !dbg !18
  call void @llvm.dbg.value(metadata i8 %0, metadata !9, metadata !DIExpression()), !dbg !18
  %1 = zext i8 %0 to i32, !dbg !19
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !19
  %2 = bitcast i32 %1 to <32 x i1>, !dbg !20
  call void @llvm.dbg.value(metadata <32 x i1> %2, metadata !13, metadata !DIExpression()), !dbg !20
  %3 = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v32i1(<32 x i1> %2, i32 0), !dbg !21
  call void @llvm.dbg.value(metadata <8 x i1> %3, metadata !15, metadata !DIExpression()), !dbg !21
  %4 = bitcast <8 x i1> %3 to i8, !dbg !22
  call void @llvm.dbg.value(metadata i8 %4, metadata !17, metadata !DIExpression()), !dbg !22
  store i8 %4, i8* %a, !dbg !23
  ret void, !dbg !24
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "simplify-rdpred.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

declare <8 x i1> @llvm.genx.rdpredregion.v8i1.v32i1(<32 x i1>, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "simplify-rdpred.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !17}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
