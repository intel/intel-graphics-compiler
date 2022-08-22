;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -GenOptLegalizer -S < %s | FileCheck %s
; ------------------------------------------------
; GenOptLegalizer
; ------------------------------------------------
; This test checks that GenOptLegalizer pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------


; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: store i32 [[VAL2_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK: store <3 x i8> {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK: store <3 x i8> {{.*}}, !dbg [[STR3_LOC:![0-9]*]]


define void @test(i24* %src1, i8* %dst1, i32* %dst2) !dbg !6 {
  %1 = load i24, i24* %src1, !dbg !16
  call void @llvm.dbg.value(metadata i24 %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = zext i24 %1 to i32, !dbg !17
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !17
  store i32 %2, i32* %dst2, !dbg !18
  %3 = load i24, i24* %src1, !dbg !19
  call void @llvm.dbg.value(metadata i24 %3, metadata !12, metadata !DIExpression()), !dbg !19
  store i24 %3, i24* %src1, !dbg !20
  %4 = bitcast i8* %dst1 to i24*, !dbg !21
  call void @llvm.dbg.value(metadata i24* %4, metadata !13, metadata !DIExpression()), !dbg !21
  %5 = trunc i32 %2 to i24, !dbg !22
  call void @llvm.dbg.value(metadata i24 %5, metadata !15, metadata !DIExpression()), !dbg !22
  store i24 %5, i24* %4, !dbg !23
  ret void, !dbg !24
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "load_store.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "load_store.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 6, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !10)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
!24 = !DILocation(line: 9, column: 1, scope: !6)
