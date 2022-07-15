;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-promoteint8type  -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : phi
; ------------------------------------------------
; This test checks that PromoteInt8Type pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_promote{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: lbl:
; CHECK: [[PHI1_V:%[A-z0-9.]*]] = phi {{.*}} !dbg [[PHI1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i8|i16}} [[PHI1_V]], metadata [[PHI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI1_LOC]]
; CHECK: end:
; CHECK: [[PHI2_V:%[A-z0-9.]*]] = phi {{.*}} !dbg [[PHI2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i8|i16}} [[PHI2_V]], metadata [[PHI2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI2_LOC]]

define void @test_promote(i8* %src1, i8* %src2) !dbg !6 {
entry:
  %0 = load i8, i8* %src1, !dbg !17
  call void @llvm.dbg.value(metadata i8 %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = icmp ugt i8 %0, 12, !dbg !18
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !18
  br i1 %1, label %lbl, label %end, !dbg !19

lbl:                                              ; preds = %lbl, %entry
  %2 = phi i8 [ %0, %entry ], [ %4, %lbl ], !dbg !20
  call void @llvm.dbg.value(metadata i8 %2, metadata !12, metadata !DIExpression()), !dbg !20
  %3 = load i8, i8* %src2, !dbg !21
  call void @llvm.dbg.value(metadata i8 %3, metadata !13, metadata !DIExpression()), !dbg !21
  %4 = add i8 %2, %3, !dbg !22
  call void @llvm.dbg.value(metadata i8 %4, metadata !14, metadata !DIExpression()), !dbg !22
  %5 = icmp eq i8 %4, 0, !dbg !23
  call void @llvm.dbg.value(metadata i1 %5, metadata !15, metadata !DIExpression()), !dbg !23
  br i1 %5, label %lbl, label %end, !dbg !24

end:                                              ; preds = %lbl, %entry
  %6 = phi i8 [ 12, %entry ], [ %4, %lbl ], !dbg !25
  call void @llvm.dbg.value(metadata i8 %6, metadata !16, metadata !DIExpression()), !dbg !25
  store i8 %6, i8* %src2, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "phi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[PHI1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[PHI1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI2_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[PHI2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "phi.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 9, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
