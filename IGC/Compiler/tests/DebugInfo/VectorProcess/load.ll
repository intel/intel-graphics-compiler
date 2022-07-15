;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vectorprocess -S < %s | FileCheck %s
; ------------------------------------------------
; VectorProcess : load
; ------------------------------------------------
; This test checks that VectorProcess pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_vectorpro
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: load{{.*}} [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <2 x i16>* [[LOAD1_V:%[A-z0-9]*]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK-DAG: [[LOAD1_V]] = {{.*}} !dbg [[LOAD1_LOC]]
; CHECK: load{{.*}} [[LOAD2_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <2 x i16*> [[LOAD2_V:%[A-z0-9]*]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK-DAG: [[LOAD2_V]] = {{.*}} !dbg [[LOAD2_LOC]]
; CHECK: load{{.*}} [[LOAD3_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <2 x i16> [[LOAD3_V:%[A-z0-9]*]], metadata [[LOAD3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD3_LOC]]
; CHECK-DAG: [[LOAD3_V]] = {{.*}} !dbg [[LOAD3_LOC]]

define void @test_vectorpro(<2 x i16>** %src1, <2 x i16*>* %src2) !dbg !6 {
  %1 = load <2 x i16>*, <2 x i16>** %src1, align 4, !dbg !18
  call void @llvm.dbg.value(metadata <2 x i16>* %1, metadata !9, metadata !DIExpression()), !dbg !18
  %2 = load <2 x i16*>, <2 x i16*>* %src2, align 1, !dbg !19
  call void @llvm.dbg.value(metadata <2 x i16*> %2, metadata !11, metadata !DIExpression()), !dbg !19
  %3 = load <2 x i16>, <2 x i16>* %1, align 2, !dbg !20
  call void @llvm.dbg.value(metadata <2 x i16> %3, metadata !13, metadata !DIExpression()), !dbg !20
  %4 = extractelement <2 x i16> %3, i32 1, !dbg !21
  call void @llvm.dbg.value(metadata i16 %4, metadata !15, metadata !DIExpression()), !dbg !21
  %5 = extractelement <2 x i16*> %2, i32 0, !dbg !22
  call void @llvm.dbg.value(metadata i16* %5, metadata !17, metadata !DIExpression()), !dbg !22
  store i16 %4, i16* %5, !dbg !23
  ret void, !dbg !24
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "load.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorpro", linkageName: "test_vectorpro", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "load.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vectorpro", linkageName: "test_vectorpro", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !17}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !16)
!16 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
