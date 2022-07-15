;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vector-bitcast-opt -S < %s | FileCheck %s
; ------------------------------------------------
; VectorBitCastOpt
; ------------------------------------------------
; This test checks that VectorBitCastOpt pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_vbitcast
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; Vector cast value is not salvageble by current llvm means
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC:![0-9]*]]
;
; CHECK: [[EXTR1_V:%[0-9]*]] = {{.*}} float
; CHECK-SAME: !dbg [[EXTR1_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[EXTR1_V]]
; CHECK-SAME: metadata [[EXTR1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR1_LOC]]
;
; CHECK: [[EXTR2_V:%[0-9]*]] = {{.*}} float
; CHECK-SAME: !dbg [[EXTR2_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[EXTR2_V]]
; CHECK-SAME: metadata [[EXTR2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR2_LOC]]
;
; CHECK: [[EXTR3_V:%[0-9]*]] = {{.*}} i32
; CHECK-SAME: !dbg [[EXTR3_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[EXTR3_V]]
; CHECK-SAME: metadata [[EXTR3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR3_LOC]]

define void @test_vbitcast(<2 x i32> %src) !dbg !6 {
  %1 = bitcast <2 x i32> %src to <2 x float>, !dbg !15
  call void @llvm.dbg.value(metadata <2 x float> %1, metadata !9, metadata !DIExpression()), !dbg !15
  %2 = extractelement <2 x float> %1, i32 1, !dbg !16
  call void @llvm.dbg.value(metadata float %2, metadata !11, metadata !DIExpression()), !dbg !16
  %3 = extractelement <2 x float> %1, i32 2, !dbg !17
  call void @llvm.dbg.value(metadata float %3, metadata !13, metadata !DIExpression()), !dbg !17
  %4 = extractelement <2 x i32> %src, i32 1, !dbg !18
  call void @llvm.dbg.value(metadata i32 %4, metadata !14, metadata !DIExpression()), !dbg !18
  ret void, !dbg !19
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "VectorBitCastOpt.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vbitcast", linkageName: "test_vbitcast", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 1, column: 1, scope:  [[SCOPE]])
; CHECK-DAG: [[EXTR1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[EXTR1_LOC]] = !DILocation(line: 2, column: 1, scope:  [[SCOPE]])
; CHECK-DAG: [[EXTR2_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[EXTR2_LOC]] = !DILocation(line: 3, column: 1, scope:  [[SCOPE]])
; CHECK-DAG: [[EXTR3_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[EXTR3_LOC]] = !DILocation(line: 4, column: 1, scope:  [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "VectorBitCastOpt.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vbitcast", linkageName: "test_vbitcast", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
