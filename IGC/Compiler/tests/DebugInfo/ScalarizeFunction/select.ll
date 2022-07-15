;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction : select operands
; ------------------------------------------------
; This test checks that ScalarizeFunction pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; Check IR:
;
; CHECK: alloca {{.*}}, !dbg [[ALLOC32_LOC:![0-9]*]]
; CHECK: dbg.declare({{.*}}, metadata [[R32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC32_LOC]]
; CHECK: alloca {{.*}}, !dbg [[ALLOC16_LOC:![0-9]*]]
; CHECK: dbg.declare({{.*}}, metadata [[R16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC16_LOC]]
; CHECK-DAG: dbg.value(metadata <2 x i32> [[SELECT32_V:%[a-z0-9\.]*]], metadata [[SELECT32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT32_LOC:![0-9]*]]
; CHECK-DAG: [[SELECT32_V]] = {{.*}}, !dbg [[SELECT32_LOC]]
; CHECK-DAG: store <2 x i32> [[SELECT32_V]], {{.*}}, !dbg [[STORE32_LOC:![0-9]*]]
;
; CHECK-DAG: dbg.value(metadata <4 x i16> [[SELECT16_V:%[a-z0-9\.]*]], metadata [[SELECT16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT16_LOC:![0-9]*]]
; CHECK-DAG: [[SELECT16_V]] = {{.*}}, !dbg [[SELECT16_LOC]]
; CHECK-DAG: store <4 x i16> [[SELECT16_V]], {{.*}}, !dbg [[STORE16_LOC:![0-9]*]]
;
; CHECK-DAG: dbg.value(metadata <4 x i16> [[SELECT16_1_V:%[a-z0-9\.]*]], metadata [[SELECT16_1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT16_1_LOC:![0-9]*]]
; CHECK-DAG: [[SELECT16_1_V]] = {{.*}}, !dbg [[SELECT16_1_LOC]]

define spir_kernel void @test_select(<2 x i32> %src1, <4 x i16> %src2, i1 %cond, <4 x i1> %vcond) !dbg !6 {
  %1 = alloca <2 x i32>, align 4, !dbg !15
  call void @llvm.dbg.declare(metadata <2 x i32>* %1, metadata !9, metadata !DIExpression()), !dbg !15
  %2 = alloca <4 x i16>, align 4, !dbg !16
  call void @llvm.dbg.declare(metadata <4 x i16>* %2, metadata !11, metadata !DIExpression()), !dbg !16
  %3 = select i1 %cond, <2 x i32> %src1, <2 x i32> <i32 42, i32 13>, !dbg !17
  call void @llvm.dbg.value(metadata <2 x i32> %3, metadata !12, metadata !DIExpression()), !dbg !17
  %4 = select <4 x i1> %vcond, <4 x i16> %src2, <4 x i16> <i16 1, i16 2, i16 3, i16 4>, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i16> %4, metadata !13, metadata !DIExpression()), !dbg !18
  %5 = select i1 %cond, <4 x i16> %src2, <4 x i16> %src2, !dbg !19
  call void @llvm.dbg.value(metadata <4 x i16> %5, metadata !14, metadata !DIExpression()), !dbg !19
  store <2 x i32> %3, <2 x i32>* %1, !dbg !20
  store <4 x i16> %4, <4 x i16>* %2, !dbg !21
  %6 = bitcast <4 x i16> %5 to i64
  ret void, !dbg !22
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ScalarizeFunction/select.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_select", linkageName: "test_select", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOC32_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOC16_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[R32_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[R16_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[SELECT32_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[SELECT32_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT16_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[SELECT16_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT16_1_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[SELECT16_1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE32_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE16_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ScalarizeFunction/select.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_select", linkageName: "test_select", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
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
!22 = !DILocation(line: 8, column: 1, scope: !6)
