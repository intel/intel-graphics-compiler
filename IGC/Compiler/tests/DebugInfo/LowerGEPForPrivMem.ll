;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-priv-mem-to-reg -S < %s | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
; This test checks that LowerGEPForPrivMem pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32>* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: store <4 x i32>{{.*}}, !dbg [[STR1_LOC:![0-9]*]]

; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK: store <4 x i32>{{.*}}, !dbg [[STR2_LOC:![0-9]*]]

; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32>* [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK: store <4 x i32>{{.*}}, !dbg [[STR3_LOC:![0-9]*]]

define void @test(<4 x i32> %a, <4 x i32>* %b) !dbg !10 {
  %1 = alloca <4 x i32>, align 4, !dbg !19, !uniform !20
  call void @llvm.dbg.value(metadata <4 x i32>* %1, metadata !13, metadata !DIExpression()), !dbg !19
  store <4 x i32> %a, <4 x i32>* %1, !dbg !21
  %2 = load <4 x i32>, <4 x i32>* %1, !dbg !22
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !15, metadata !DIExpression()), !dbg !22
  store <4 x i32> %2, <4 x i32>* %b, !dbg !23
  %3 = alloca [4 x i32], align 4, !dbg !24, !uniform !20
  call void @llvm.dbg.value(metadata [4 x i32]* %3, metadata !17, metadata !DIExpression()), !dbg !24
  %4 = getelementptr [4 x i32], [4 x i32]* %3, i32 0, i32 3, !dbg !25
  call void @llvm.dbg.value(metadata i32* %4, metadata !18, metadata !DIExpression()), !dbg !25
  store i32 13, i32* %4, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "LowerGEPForPrivMem.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])



; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (<4 x i32>, <4 x i32>*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "LowerGEPForPrivMem.ll", directory: "/")
!6 = !{}
!7 = !{i32 8}
!8 = !{i32 4}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 3, type: !16)
!16 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 5, type: !14)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 6, type: !14)
!19 = !DILocation(line: 1, column: 1, scope: !10)
!20 = !{i1 true}
!21 = !DILocation(line: 2, column: 1, scope: !10)
!22 = !DILocation(line: 3, column: 1, scope: !10)
!23 = !DILocation(line: 4, column: 1, scope: !10)
!24 = !DILocation(line: 5, column: 1, scope: !10)
!25 = !DILocation(line: 6, column: 1, scope: !10)
!26 = !DILocation(line: 7, column: 1, scope: !10)
!27 = !DILocation(line: 8, column: 1, scope: !10)
