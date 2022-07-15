;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-shuffle-simplification -S < %s | FileCheck %s
; ------------------------------------------------
; GenSimplification
; ------------------------------------------------
; This test checks that GenSimplification pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: end:
; CHECK-DAG: store i32 [[VAL7_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[PHI1_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[VAL8_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[PHI2_LOC:![0-9]*]]

define void @test(i32 %a, i32* %b, i1 %c) !dbg !9 {
entry:
  %0 = insertelement <2 x i32> undef, i32 %a, i32 0, !dbg !23
  call void @llvm.dbg.value(metadata <2 x i32> %0, metadata !12, metadata !DIExpression()), !dbg !23
  br i1 %c, label %bb1, label %bb2, !dbg !24

bb1:                                              ; preds = %entry
  %1 = insertelement <2 x i32> %0, i32 13, i32 1, !dbg !25
  call void @llvm.dbg.value(metadata <2 x i32> %1, metadata !14, metadata !DIExpression()), !dbg !25
  %2 = bitcast i32 15 to <2 x i16>, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i16> %2, metadata !15, metadata !DIExpression()), !dbg !26
  br label %end, !dbg !27

bb2:                                              ; preds = %entry
  %3 = insertelement <2 x i32> %0, i32 32, i32 1, !dbg !28
  call void @llvm.dbg.value(metadata <2 x i32> %3, metadata !17, metadata !DIExpression()), !dbg !28
  %4 = bitcast i32 32 to <2 x i16>, !dbg !29
  call void @llvm.dbg.value(metadata <2 x i16> %4, metadata !18, metadata !DIExpression()), !dbg !29
  br label %end, !dbg !30

end:                                              ; preds = %bb2, %bb1
  %5 = phi <2 x i32> [ %1, %bb1 ], [ %3, %bb2 ], !dbg !31
  %6 = phi <2 x i16> [ %2, %bb1 ], [ %4, %bb2 ], !dbg !32
  call void @llvm.dbg.value(metadata <2 x i32> %5, metadata !19, metadata !DIExpression()), !dbg !31
  call void @llvm.dbg.value(metadata <2 x i16> %6, metadata !20, metadata !DIExpression()), !dbg !32
  %7 = extractelement <2 x i32> %5, i32 1, !dbg !33
  call void @llvm.dbg.value(metadata i32 %7, metadata !21, metadata !DIExpression()), !dbg !33
  store i32 %7, i32* %b, !dbg !34
  %8 = bitcast <2 x i16> %6 to i32, !dbg !35
  call void @llvm.dbg.value(metadata i32 %8, metadata !22, metadata !DIExpression()), !dbg !35
  store i32 %8, i32* %b, !dbg !36
  ret void, !dbg !37
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "phi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[PHI1_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI2_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i32, i32*, i1)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 2}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "phi.ll", directory: "/")
!5 = !{}
!6 = !{i32 15}
!7 = !{i32 9}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !17, !18, !19, !20, !21, !22}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 3, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 4, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 6, type: !13)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 7, type: !16)
!19 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 9, type: !13)
!20 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 10, type: !16)
!21 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 11, type: !16)
!22 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 13, type: !16)
!23 = !DILocation(line: 1, column: 1, scope: !9)
!24 = !DILocation(line: 2, column: 1, scope: !9)
!25 = !DILocation(line: 3, column: 1, scope: !9)
!26 = !DILocation(line: 4, column: 1, scope: !9)
!27 = !DILocation(line: 5, column: 1, scope: !9)
!28 = !DILocation(line: 6, column: 1, scope: !9)
!29 = !DILocation(line: 7, column: 1, scope: !9)
!30 = !DILocation(line: 8, column: 1, scope: !9)
!31 = !DILocation(line: 9, column: 1, scope: !9)
!32 = !DILocation(line: 10, column: 1, scope: !9)
!33 = !DILocation(line: 11, column: 1, scope: !9)
!34 = !DILocation(line: 12, column: 1, scope: !9)
!35 = !DILocation(line: 13, column: 1, scope: !9)
!36 = !DILocation(line: 14, column: 1, scope: !9)
!37 = !DILocation(line: 15, column: 1, scope: !9)
