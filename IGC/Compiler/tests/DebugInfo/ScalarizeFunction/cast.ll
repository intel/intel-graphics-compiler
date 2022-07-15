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
; ScalarizeFunction : castInst operands
; ------------------------------------------------
; This test checks that ScalarizeFunction pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; Check IR:
;
; CHECK: alloca {{.*}}, !dbg [[ALLOC64_LOC:![0-9]*]]
; CHECK: dbg.declare({{.*}}, metadata [[R64_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC64_LOC]]
; CHECK: alloca {{.*}}, !dbg [[ALLOC16_LOC:![0-9]*]]
; CHECK: dbg.declare({{.*}}, metadata [[R16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC16_LOC]]
; CHECK-DAG: dbg.value(metadata <2 x i64> [[CAST_V:%[a-z0-9\.]*]], metadata [[CAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CAST_LOC:![0-9]*]]
; CHECK-DAG: [[CAST_V]] = {{.*}}, !dbg [[CAST_LOC]]
; CHECK-DAG: store <2 x i64> [[CAST_V]], {{.*}}, !dbg [[STORE64_LOC:![0-9]*]]
;
; CHECK-DAG: dbg.value(metadata <4 x i16> [[BITCAST_V:%[a-z0-9\.]*]], metadata [[BITCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BITCAST_LOC:![0-9]*]]
; CHECK-DAG: [[BITCAST_V]] = {{.*}}, !dbg [[BITCAST_LOC]]
; CHECK-DAG: store <4 x i16> [[BITCAST_V]], {{.*}}, !dbg [[STORE16_LOC:![0-9]*]]

define spir_kernel void @test_cast(<2 x i32> %src1) !dbg !6 {
  %1 = alloca <2 x i64>, align 4, !dbg !15
  call void @llvm.dbg.declare(metadata <2 x i64>* %1, metadata !9, metadata !DIExpression()), !dbg !15
  %2 = alloca <4 x i16>, align 4, !dbg !16
  call void @llvm.dbg.declare(metadata <4 x i16>* %2, metadata !11, metadata !DIExpression()), !dbg !16
  %3 = sext <2 x i32> %src1 to <2 x i64>, !dbg !17
  call void @llvm.dbg.value(metadata <2 x i64> %3, metadata !12, metadata !DIExpression()), !dbg !17
  %4 = bitcast <2 x i32> %src1 to <4 x i16>, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i16> %4, metadata !14, metadata !DIExpression()), !dbg !18
  store <2 x i64> %3, <2 x i64>* %1, !dbg !19
  store <4 x i16> %4, <4 x i16>* %2, !dbg !20
  ret void, !dbg !21
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ScalarizeFunction/cast.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_cast", linkageName: "test_cast", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOC64_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOC16_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[R64_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[R16_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CAST_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CAST_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BITCAST_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[BITCAST_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE64_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE16_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ScalarizeFunction/cast.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_cast", linkageName: "test_cast", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
!20 = !DILocation(line: 6, column: 1, scope: !6)
!21 = !DILocation(line: 7, column: 1, scope: !6)
