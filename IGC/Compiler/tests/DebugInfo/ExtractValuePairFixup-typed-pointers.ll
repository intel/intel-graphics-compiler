;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-extractvalue-pair-fixup -S < %s | FileCheck %s
; ------------------------------------------------
; ExtractValuePairFixup
; ------------------------------------------------
; This test checks that ExtractValuePairFixup pass follows
; 'How to Update Debug Info' llvm guideline.
;
; ------------------------------------------------

%str = type { i32, i64 }

; CHECK: @test_extr{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[IV1_V:%[0-9]*]] = insertvalue{{.*}} !dbg [[IV1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata{{.*}} [[IV1_V]], metadata [[IV1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IV1_LOC]]
; CHECK: [[IV2_V:%[0-9]*]] = insertvalue{{.*}} !dbg [[IV2_LOC:![0-9]*]]
;
; extractvalue is moved somewhere in this BB.
;
; CHECK-DAG: @llvm.dbg.value(metadata{{.*}} [[IV2_V]], metadata [[IV2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IV2_LOC]]
; CHECK-DAG: [[EV1_V:%[0-9]*]] = extractvalue %str [[IV2_V]], 1, !dbg [[EV1_LOC:![0-9]*]]
; CHECK-DAG: [[ALLOCA_V:%[0-9]*]] = alloca {{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
;
; but dbg calls should remain in same order
;
; CHECK: @llvm.dbg.value(metadata i64* [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[EV1_V]], metadata [[EV1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EV1_LOC]]
; check that nothing moved between BBs.
; CHECK: lbl:
; CHECK: [[IV3_V:%[0-9]*]] = insertvalue{{.*}} !dbg [[IV3_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata{{.*}} [[IV3_V]], metadata [[IV3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IV3_LOC]]
; CHECK-DAG: [[EV2_V:%[0-9]*]] = extractvalue %str [[IV3_V]], 1, !dbg [[EV2_LOC:![0-9]*]]
; CHECK-DAG: [[SEXT_V:%[0-9]*]] = sext {{.*}} !dbg [[SEXT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i64 [[SEXT_V]], metadata [[SEXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SEXT_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[EV2_V]], metadata [[EV2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EV2_LOC]]

define spir_kernel void @test_extr(i32 %src1, i64 %src2) !dbg !9 {
entry:
  %0 = insertvalue %str { i32 0, i64 32 }, i32 %src1, 0, !dbg !23
  call void @llvm.dbg.value(metadata %str %0, metadata !12, metadata !DIExpression()), !dbg !23
  %1 = insertvalue %str %0, i64 %src2, 1, !dbg !24
  call void @llvm.dbg.value(metadata %str %1, metadata !14, metadata !DIExpression()), !dbg !24
  %2 = alloca i64, align 8, !dbg !25
  call void @llvm.dbg.value(metadata i64* %2, metadata !15, metadata !DIExpression()), !dbg !25
  %3 = extractvalue %str %1, 1, !dbg !26
  call void @llvm.dbg.value(metadata i64 %3, metadata !17, metadata !DIExpression()), !dbg !26
  %4 = add i64 %src2, %3, !dbg !27
  call void @llvm.dbg.value(metadata i64 %4, metadata !18, metadata !DIExpression()), !dbg !27
  br label %lbl, !dbg !28

lbl:                                              ; preds = %entry
  %5 = insertvalue %str %1, i64 %3, 1, !dbg !29
  call void @llvm.dbg.value(metadata %str %5, metadata !19, metadata !DIExpression()), !dbg !29
  %6 = sext i32 %src1 to i64, !dbg !30
  call void @llvm.dbg.value(metadata i64 %6, metadata !20, metadata !DIExpression()), !dbg !30
  %7 = extractvalue %str %5, 1, !dbg !31
  call void @llvm.dbg.value(metadata i64 %7, metadata !21, metadata !DIExpression()), !dbg !31
  %8 = add i64 %7, %6, !dbg !32
  call void @llvm.dbg.value(metadata i64 %8, metadata !22, metadata !DIExpression()), !dbg !32
  store i64 %8, i64* %2, !dbg !33
  ret void, !dbg !34
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ExtractValuePairFixup.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_extr", linkageName: "test_extr", scope: null, file: !4, line: 1
; CHECK-DAG: [[IV1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[IV1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IV2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[IV2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EV1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[EV1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IV3_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[IV3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SEXT_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[SEXT_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EV2_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[EV2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i32, i64)* @test_extr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "ExtractValuePairFixup.ll", directory: "/")
!5 = !{}
!6 = !{i32 12}
!7 = !{i32 9}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_extr", linkageName: "test_extr", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !17, !18, !19, !20, !21, !22}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty96", size: 96, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !16)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !16)
!19 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 7, type: !13)
!20 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 8, type: !16)
!21 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 9, type: !16)
!22 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 10, type: !16)
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
