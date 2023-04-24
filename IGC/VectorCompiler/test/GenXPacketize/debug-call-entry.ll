;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPacketize
; ------------------------------------------------
; This test checks that GenXPacketize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_packetize{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <8 x float> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <8 x i32> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; Sqrt and fabs are values are salvageble(at least for some cases)
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <8 x float> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <8 x float> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
;
; CHECK-DAG: void @llvm.dbg.value(metadata <8 x float> [[VAL5_V:%[A-z0-9.]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]

define void @test_packetize(<8 x float>* %a, <8 x i32>* %b) #0 !dbg !6 {
entry:
  %0 = load <8 x float>, <8 x float>* %a, !dbg !15
  call void @llvm.dbg.value(metadata <8 x float> %0, metadata !9, metadata !DIExpression()), !dbg !15
  %1 = load <8 x i32>, <8 x i32>* %b, !dbg !16
  call void @llvm.dbg.value(metadata <8 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !16
  %2 = call <8 x float> @sqrtf(<8 x float> %0), !dbg !17
  call void @llvm.dbg.value(metadata <8 x float> %2, metadata !12, metadata !DIExpression()), !dbg !17
  %3 = call <8 x float> @fabs(<8 x float> %2), !dbg !18
  call void @llvm.dbg.value(metadata <8 x float> %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = call <8 x float> @exp2(<8 x float> %3), !dbg !19
  call void @llvm.dbg.value(metadata <8 x float> %4, metadata !14, metadata !DIExpression()), !dbg !19
  store <8 x float> %4, <8 x float>* %a, !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPacketize.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: [[FILE]], line: 1
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

declare <8 x float> @sqrtf(<8 x float>)

declare <8 x float> @fabs(<8 x float>)

declare <8 x float> @exp2(<8 x float>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxSIMT"="8" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXPacketize.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
!20 = !DILocation(line: 6, column: 1, scope: !6)
!21 = !DILocation(line: 7, column: 1, scope: !6)
