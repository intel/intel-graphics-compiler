;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXUnbalingWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXUnbaling
; ------------------------------------------------
; This test checks that GenXUnbaling pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define void @test_unbale{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i8> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i8> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i8> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i8> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i8> [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <8 x i8> [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i8> [[VAL6_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]

define void @test_unbale(<4 x i32>* %a, <16 x i8>* %b, <8 x i8>* %c) #0 !dbg !6 {
entry:
  %0 = load <4 x i32>, <4 x i32>* %a, !dbg !20
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !20
  %1 = load <16 x i8>, <16 x i8>* %b, !dbg !21
  call void @llvm.dbg.value(metadata <16 x i8> %1, metadata !11, metadata !DIExpression()), !dbg !21
  %2 = bitcast <4 x i32> %0 to <16 x i8>, !dbg !22
  call void @llvm.dbg.value(metadata <16 x i8> %2, metadata !12, metadata !DIExpression()), !dbg !22
  %3 = call <4 x i8> @llvm.genx.rdregioni.v4i8.v16i8.i16(<16 x i8> %1, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !23
  call void @llvm.dbg.value(metadata <4 x i8> %3, metadata !13, metadata !DIExpression()), !dbg !23
  %4 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v4i8.i8.i1(<16 x i8> %2, <4 x i8> %3, i32 1, i32 1, i32 0, i16 8, i32 0, i1 true), !dbg !24
  call void @llvm.dbg.value(metadata <16 x i8> %4, metadata !15, metadata !DIExpression()), !dbg !24
  %5 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v4i8.i8.i1(<16 x i8> %4, <4 x i8> %3, i32 1, i32 1, i32 0, i16 12, i32 0, i1 true), !dbg !25
  call void @llvm.dbg.value(metadata <16 x i8> %5, metadata !16, metadata !DIExpression()), !dbg !25
  %6 = call <8 x i8> @llvm.genx.rdregioni.v8i8.v16i8.i16(<16 x i8> %2, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !26
  call void @llvm.dbg.value(metadata <8 x i8> %6, metadata !17, metadata !DIExpression()), !dbg !26
  %7 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v8i8.i8.i1(<16 x i8> %5, <8 x i8> %6, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true), !dbg !27
  call void @llvm.dbg.value(metadata <16 x i8> %7, metadata !19, metadata !DIExpression()), !dbg !27
  store <16 x i8> %7, <16 x i8>* %b, !dbg !28
  store <8 x i8> %6, <8 x i8>* %c, !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "process-nonover.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_unbale", linkageName: "test_unbale", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

declare <4 x i8> @llvm.genx.rdregioni.v4i8.v16i8.i16(<16 x i8>, i32, i32, i32, i16, i32)

declare <8 x i8> @llvm.genx.rdregioni.v8i8.v16i8.i16(<16 x i8>, i32, i32, i32, i16, i32)

declare <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v4i8.i8.i1(<16 x i8>, <4 x i8>, i32, i32, i32, i16, i32, i1)

declare <16 x i8> @llvm.genx.wrregioni.v16i8.v16i8.v8i8.i8.i1(<16 x i8>, <8 x i8>, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "process-nonover.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_unbale", linkageName: "test_unbale", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16, !17, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !18)
!18 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)
!28 = !DILocation(line: 9, column: 1, scope: !6)
!29 = !DILocation(line: 10, column: 1, scope: !6)
!30 = !DILocation(line: 11, column: 1, scope: !6)
