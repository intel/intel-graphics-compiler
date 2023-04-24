;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXTidyControlFlow -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXTidyControlFlow
; ------------------------------------------------
; This test checks that GenXTidyControlFlow pass follows
; 'How to Update Debug Info' llvm guideline.

; CHECK: define dllexport void @test_tidycf{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: b1:
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-NOT: b2:
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: end:
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]

define dllexport void @test_tidycf(i32* %a) #1 !dbg !6 {
entry:
  %0 = load i32, i32* %a, !dbg !16
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = add i32 %0, %0, !dbg !17
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !17
  %2 = icmp slt i32 %1, 123, !dbg !18
  call void @llvm.dbg.value(metadata i1 %2, metadata !12, metadata !DIExpression()), !dbg !18
  br i1 %2, label %b1, label %end, !dbg !19

b1:                                               ; preds = %entry
  %3 = add i32 %0, %1, !dbg !20
  br label %b2, !dbg !21

b2:                                               ; preds = %b1
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !20
  call void @llvm.dbg.value(metadata i32 %3, metadata !22, metadata !DIExpression()), !dbg !21
  br label %b3, !dbg !23

b3:                                               ; preds = %b2
  store i32 %3, i32* %a
  br label %end, !dbg !23

end:                                              ; preds = %b3, %entry
  %4 = phi i32 [ %3, %b3 ], [ %1, %entry ], !dbg !24
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !24
  store i32 %4, i32* %a, !dbg !25
  ret void, !dbg !26
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "remove-blocks.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_tidycf", linkageName: "test_tidycf", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }
attributes #1 = { "CMGenxMain" }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "remove-blocks.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_tidycf", linkageName: "test_tidycf", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 8, type: !10)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
