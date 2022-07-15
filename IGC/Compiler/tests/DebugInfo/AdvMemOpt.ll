;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-advmemopt -S < %s | FileCheck %s
; ------------------------------------------------
; AdvMemOpt
; ------------------------------------------------
; This test checks that AdvMemOpt pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]

; CHECK: body:
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
;
; CHECK: body1:
; CHECK: void @llvm.dbg.value(metadata i32 %c, metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]

define void @test(i32 %a, i32* %b) !dbg !10 {
entry:
  %0 = icmp slt i32 %a, 13, !dbg !23
  call void @llvm.dbg.value(metadata i1 %0, metadata !13, metadata !DIExpression()), !dbg !23
  br i1 %0, label %body, label %end, !dbg !24

body:                                             ; preds = %body1, %entry
  %1 = phi i32 [ 0, %entry ], [ %3, %body1 ], !dbg !25
  call void @llvm.dbg.value(metadata i32 %1, metadata !15, metadata !DIExpression()), !dbg !25
  %2 = load i32, i32* %b, align 4, !dbg !26, !uniform !27
  call void @llvm.dbg.value(metadata i32 %2, metadata !17, metadata !DIExpression()), !dbg !26
  br label %body1, !dbg !28

body1:                                            ; preds = %body
  %c = load i32, i32* %b, align 4, !dbg !29, !uniform !27
  call void @llvm.dbg.value(metadata i32 %c, metadata !18, metadata !DIExpression()), !dbg !29
  %3 = add i32 %1, %2, !dbg !30
  call void @llvm.dbg.value(metadata i32 %3, metadata !19, metadata !DIExpression()), !dbg !30
  %4 = add i32 %c, %3, !dbg !31
  call void @llvm.dbg.value(metadata i32 %4, metadata !20, metadata !DIExpression()), !dbg !31
  %5 = icmp slt i32 %3, %a, !dbg !32
  call void @llvm.dbg.value(metadata i1 %5, metadata !21, metadata !DIExpression()), !dbg !32
  br i1 %5, label %body, label %end, !dbg !33

end:                                              ; preds = %body1, %entry
  %6 = phi i32 [ %a, %entry ], [ %4, %body1 ], !dbg !34
  call void @llvm.dbg.value(metadata i32 %6, metadata !22, metadata !DIExpression()), !dbg !34
  store i32 %6, i32* %b, !dbg !35
  ret void, !dbg !36
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "AdvMemOpt.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])


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
!5 = !DIFile(filename: "AdvMemOpt.ll", directory: "/")
!6 = !{}
!7 = !{i32 13}
!8 = !{i32 8}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18, !19, !20, !21, !22}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 3, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 4, type: !16)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 6, type: !16)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 7, type: !16)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 8, type: !16)
!21 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 9, type: !14)
!22 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 11, type: !16)
!23 = !DILocation(line: 1, column: 1, scope: !10)
!24 = !DILocation(line: 2, column: 1, scope: !10)
!25 = !DILocation(line: 3, column: 1, scope: !10)
!26 = !DILocation(line: 4, column: 1, scope: !10)
!27 = !{i1 true}
!28 = !DILocation(line: 5, column: 1, scope: !10)
!29 = !DILocation(line: 6, column: 1, scope: !10)
!30 = !DILocation(line: 7, column: 1, scope: !10)
!31 = !DILocation(line: 8, column: 1, scope: !10)
!32 = !DILocation(line: 9, column: 1, scope: !10)
!33 = !DILocation(line: 10, column: 1, scope: !10)
!34 = !DILocation(line: 11, column: 1, scope: !10)
!35 = !DILocation(line: 12, column: 1, scope: !10)
!36 = !DILocation(line: 13, column: 1, scope: !10)
