;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-phielimination -S < %s | FileCheck %s
; ------------------------------------------------
; DeadPHINodeElimination
; ------------------------------------------------
; This test checks that DeadPHINodeElimination pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: body:
; CHECK: [[Y1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[Y1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[Y1_V]], metadata [[X1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[X1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[Y1_V]], metadata [[Y1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[Y1_LOC:![0-9]*]]

; CHECK: body1:
; CHECK: [[Y2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[Y2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[Y2_V]], metadata [[X2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[X2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[Y2_V]], metadata [[Y2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[Y2_LOC:![0-9]*]]

; CHECK: body2:
; CHECK: [[Y3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[Y3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[Y3_V]], metadata [[X3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[X3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[Y3_V]], metadata [[Y3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[Y3_LOC:![0-9]*]]

define void @test(i32 %a, i32* %b) !dbg !10 {
entry:
  %0 = icmp slt i32 %a, 13, !dbg !28
  call void @llvm.dbg.value(metadata i1 %0, metadata !13, metadata !DIExpression()), !dbg !28
  %1 = load i32, i32* %b, !dbg !29
  call void @llvm.dbg.value(metadata i32 %1, metadata !15, metadata !DIExpression()), !dbg !29
  br i1 %0, label %body, label %end, !dbg !30

body:                                             ; preds = %body1, %entry
  %x1 = phi i32 [ 0, %entry ], [ %x2, %body1 ], !dbg !31
  %y1 = phi i32 [ 0, %entry ], [ %y2, %body1 ], !dbg !32
  call void @llvm.dbg.value(metadata i32 %x1, metadata !17, metadata !DIExpression()), !dbg !31
  call void @llvm.dbg.value(metadata i32 %y1, metadata !18, metadata !DIExpression()), !dbg !32
  %2 = icmp slt i32 %a, 16, !dbg !33
  call void @llvm.dbg.value(metadata i1 %2, metadata !19, metadata !DIExpression()), !dbg !33
  br i1 %2, label %body1, label %body2, !dbg !34

body1:                                            ; preds = %body2, %body
  %x2 = phi i32 [ %x1, %body ], [ %x3, %body2 ], !dbg !35
  %y2 = phi i32 [ %y1, %body ], [ %y3, %body2 ], !dbg !36
  call void @llvm.dbg.value(metadata i32 %x2, metadata !20, metadata !DIExpression()), !dbg !35
  call void @llvm.dbg.value(metadata i32 %y2, metadata !21, metadata !DIExpression()), !dbg !36
  %3 = add i32 %x2, %1, !dbg !37
  call void @llvm.dbg.value(metadata i32 %3, metadata !22, metadata !DIExpression()), !dbg !37
  %4 = icmp slt i32 %3, %a, !dbg !38
  call void @llvm.dbg.value(metadata i1 %4, metadata !23, metadata !DIExpression()), !dbg !38
  br i1 %4, label %body, label %body2, !dbg !39

body2:                                            ; preds = %body1, %body
  %x3 = phi i32 [ %x1, %body ], [ %x2, %body1 ], !dbg !40
  %y3 = phi i32 [ %y1, %body ], [ %y2, %body1 ], !dbg !41
  call void @llvm.dbg.value(metadata i32 %x3, metadata !24, metadata !DIExpression()), !dbg !40
  call void @llvm.dbg.value(metadata i32 %y3, metadata !25, metadata !DIExpression()), !dbg !41
  %5 = icmp slt i32 %a, 25, !dbg !42
  call void @llvm.dbg.value(metadata i1 %5, metadata !26, metadata !DIExpression()), !dbg !42
  br i1 %5, label %body1, label %end, !dbg !43

end:                                              ; preds = %body2, %entry
  %6 = phi i32 [ %a, %entry ], [ %y3, %body2 ], !dbg !44
  call void @llvm.dbg.value(metadata i32 %6, metadata !27, metadata !DIExpression()), !dbg !44
  store i32 %6, i32* %b, !dbg !45
  ret void, !dbg !46
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "DeadPHINodeElimination.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[X1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[X1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[Y1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[Y1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[X2_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[X2_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[Y2_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[Y2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[X3_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[X3_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[Y3_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[Y3_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32, i32*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "DeadPHINodeElimination.ll", directory: "/")
!6 = !{}
!7 = !{i32 19}
!8 = !{i32 13}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 4, type: !16)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 5, type: !16)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !14)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 8, type: !16)
!21 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 9, type: !16)
!22 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 10, type: !16)
!23 = !DILocalVariable(name: "9", scope: !10, file: !5, line: 11, type: !14)
!24 = !DILocalVariable(name: "10", scope: !10, file: !5, line: 13, type: !16)
!25 = !DILocalVariable(name: "11", scope: !10, file: !5, line: 14, type: !16)
!26 = !DILocalVariable(name: "12", scope: !10, file: !5, line: 15, type: !14)
!27 = !DILocalVariable(name: "13", scope: !10, file: !5, line: 17, type: !16)
!28 = !DILocation(line: 1, column: 1, scope: !10)
!29 = !DILocation(line: 2, column: 1, scope: !10)
!30 = !DILocation(line: 3, column: 1, scope: !10)
!31 = !DILocation(line: 4, column: 1, scope: !10)
!32 = !DILocation(line: 5, column: 1, scope: !10)
!33 = !DILocation(line: 6, column: 1, scope: !10)
!34 = !DILocation(line: 7, column: 1, scope: !10)
!35 = !DILocation(line: 8, column: 1, scope: !10)
!36 = !DILocation(line: 9, column: 1, scope: !10)
!37 = !DILocation(line: 10, column: 1, scope: !10)
!38 = !DILocation(line: 11, column: 1, scope: !10)
!39 = !DILocation(line: 12, column: 1, scope: !10)
!40 = !DILocation(line: 13, column: 1, scope: !10)
!41 = !DILocation(line: 14, column: 1, scope: !10)
!42 = !DILocation(line: 15, column: 1, scope: !10)
!43 = !DILocation(line: 16, column: 1, scope: !10)
!44 = !DILocation(line: 17, column: 1, scope: !10)
!45 = !DILocation(line: 18, column: 1, scope: !10)
!46 = !DILocation(line: 19, column: 1, scope: !10)
