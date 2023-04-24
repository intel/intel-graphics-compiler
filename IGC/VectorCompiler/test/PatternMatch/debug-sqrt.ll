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
; CHECK: void @llvm.dbg.value(metadata <16 x float> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; sqrt optimized
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <16 x float> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

define void @test_patternmatch(<16 x float>* %a) !dbg !6 {
  %1 = load <16 x float>, <16 x float>* %a, !dbg !13
  call void @llvm.dbg.value(metadata <16 x float> %1, metadata !9, metadata !DIExpression()), !dbg !13
  %2 = call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %1), !dbg !14
  call void @llvm.dbg.value(metadata <16 x float> %2, metadata !11, metadata !DIExpression()), !dbg !14
  %3 = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %2), !dbg !15
  call void @llvm.dbg.value(metadata <16 x float> %3, metadata !12, metadata !DIExpression()), !dbg !15
  store <16 x float> %3, <16 x float>* %a, !dbg !16
  ret void, !dbg !17
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "sqrt.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

declare <16 x float> @llvm.genx.sqrt.v16f32(<16 x float>)

declare <16 x float> @llvm.genx.inv.v16f32(<16 x float>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "sqrt.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
