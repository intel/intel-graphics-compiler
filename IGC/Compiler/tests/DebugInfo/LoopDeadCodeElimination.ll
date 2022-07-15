;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-loop-dce -S < %s | FileCheck %s
; ------------------------------------------------
; LoopDeadCodeElimination
; ------------------------------------------------
; This test checks that LoopDeadCodeElimination pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------


; CHECK: define spir_kernel void @test_loopdce{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]

; CHECK: bb1:
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]

; CHECK: end:
; CHECK: [[VAL6_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]

define spir_kernel void @test_loopdce(i32 %a, i32 %b, i32* %c) !dbg !9 {
entry:
  %0 = add i32 %a, %b, !dbg !20
  call void @llvm.dbg.value(metadata i32 %0, metadata !12, metadata !DIExpression()), !dbg !20
  br label %bb1, !dbg !21

bb1:                                              ; preds = %bb1, %entry
  %1 = phi i32 [ %a, %entry ], [ %4, %bb1 ], !dbg !22
  call void @llvm.dbg.value(metadata i32 %1, metadata !14, metadata !DIExpression()), !dbg !22
  %2 = icmp sgt i32 %1, 13, !dbg !23
  call void @llvm.dbg.value(metadata i1 %2, metadata !15, metadata !DIExpression()), !dbg !23
  %3 = add i32 %1, 13, !dbg !24
  call void @llvm.dbg.value(metadata i32 %3, metadata !17, metadata !DIExpression()), !dbg !24
  %4 = select i1 %2, i32 %3, i32 %0, !dbg !25
  call void @llvm.dbg.value(metadata i32 %4, metadata !18, metadata !DIExpression()), !dbg !25
  br i1 %2, label %bb1, label %end, !dbg !26

end:                                              ; preds = %bb1
  %5 = add i32 %4, %b, !dbg !27
  call void @llvm.dbg.value(metadata i32 %5, metadata !19, metadata !DIExpression()), !dbg !27
  store i32 %5, i32* %c, !dbg !28
  ret void, !dbg !29
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "LoopDeadCodeElimination.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_loopdce", linkageName: "test_loopdce", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i32, i32, i32*)* @test_loopdce, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "LoopDeadCodeElimination.ll", directory: "/")
!5 = !{}
!6 = !{i32 10}
!7 = !{i32 6}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_loopdce", linkageName: "test_loopdce", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !17, !18, !19}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 3, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 4, type: !16)
!16 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 5, type: !13)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 6, type: !13)
!19 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 8, type: !13)
!20 = !DILocation(line: 1, column: 1, scope: !9)
!21 = !DILocation(line: 2, column: 1, scope: !9)
!22 = !DILocation(line: 3, column: 1, scope: !9)
!23 = !DILocation(line: 4, column: 1, scope: !9)
!24 = !DILocation(line: 5, column: 1, scope: !9)
!25 = !DILocation(line: 6, column: 1, scope: !9)
!26 = !DILocation(line: 7, column: 1, scope: !9)
!27 = !DILocation(line: 8, column: 1, scope: !9)
!28 = !DILocation(line: 9, column: 1, scope: !9)
!29 = !DILocation(line: 10, column: 1, scope: !9)
