;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXDeadVectorRemoval
; ------------------------------------------------
; This test checks that GenXDeadVectorRemoval pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; This pass sets dead values to undef, check that live are not affected.

; CHECK: void @test_dead{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32>* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK-DAG: store <4 x i32> [[VAL6_V:%[A-z0-9]*]], <4 x i32>* [[VAL1_V]]{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}(<4 x i32> [[VAL5_V:%[A-z0-9.]*]],{{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]

define void @test_dead(<4 x i32> %b) !dbg !6 {
entry:
  %0 = alloca <4 x i32>, !dbg !17
  call void @llvm.dbg.value(metadata <4 x i32>* %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = add <4 x i32> %b, <i32 13, i32 14, i32 15, i32 16>, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !18
  %2 = add <4 x i32> %1, <i32 13, i32 14, i32 15, i32 16>, !dbg !19
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !13, metadata !DIExpression()), !dbg !19
  %3 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %b, i32 1, i32 1, i32 0, i16 8, i32 0) #0, !dbg !20
  call void @llvm.dbg.value(metadata <2 x i32> %3, metadata !14, metadata !DIExpression()), !dbg !20
  %4 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.v4i32.i16.i1(<4 x i32> %b, <2 x i32> %3, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true), !dbg !21
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !15, metadata !DIExpression()), !dbg !21
  %5 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.v4i32.i16.i1(<4 x i32> %4, <2 x i32> %3, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true), !dbg !22
  call void @llvm.dbg.value(metadata <4 x i32> %5, metadata !16, metadata !DIExpression()), !dbg !22
  store <4 x i32> %5, <4 x i32>* %0, !dbg !23
  ret void, !dbg !24
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXDeadVectorRemoval.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_dead", linkageName: "test_dead", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.v4i32.i16.i1(<4 x i32>, <2 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXDeadVectorRemoval.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_dead", linkageName: "test_dead", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
