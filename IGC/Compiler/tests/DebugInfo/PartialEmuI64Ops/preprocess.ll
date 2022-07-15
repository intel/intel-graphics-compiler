;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-PartialEmuI64Ops -S < %s | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : preprocess part
; ------------------------------------------------
; This test checks that PartialEmuI64Ops pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_part64{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[CALL1_V:%[A-z0-9]*]], metadata [[CALL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL1_LOC:![0-9]*]]
; CHECK-DAG: [[CALL1_V]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i64 [[CALL64_V:%[A-z0-9]*]], metadata [[CALL64_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL64_LOC:![0-9]*]]
; CHECK-DAG: [[CALL64_V]] = {{.*}}, !dbg [[CALL_LOC]]
; CHECK: [[IPTR_V:%[0-9]*]] = inttoptr {{.*}}, !dbg [[IPTR_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64* [[IPTR_V:%[A-z0-9]*]], metadata [[IPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IPTR_LOC:![0-9]*]]
; CHECK: [[PTRI_V:%[0-9]*]] = ptrtoint {{.*}}, !dbg [[PTRI_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 [[PTRI_V:%[A-z0-9]*]], metadata [[PTRI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTRI_LOC:![0-9]*]]

define void @test_part64(i64 %a, i64 %b, i64* %dst) !dbg !9 {
  %1 = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b), !dbg !24
  call void @llvm.dbg.value(metadata { i64, i1 } %1, metadata !12, metadata !DIExpression()), !dbg !24
  %2 = extractvalue { i64, i1 } %1, 1, !dbg !25
  call void @llvm.dbg.value(metadata i1 %2, metadata !14, metadata !DIExpression()), !dbg !25
  %3 = extractvalue { i64, i1 } %1, 0, !dbg !26
  call void @llvm.dbg.value(metadata i64 %3, metadata !16, metadata !DIExpression()), !dbg !26
  %4 = zext i1 %2 to i64, !dbg !27
  call void @llvm.dbg.value(metadata i64 %4, metadata !18, metadata !DIExpression()), !dbg !27
  store i64 %3, i64* %dst, !dbg !28
  store i64 %4, i64* %dst, !dbg !29
  %5 = load i64, i64* %dst, !dbg !30
  call void @llvm.dbg.value(metadata i64 %5, metadata !19, metadata !DIExpression()), !dbg !30
  %6 = inttoptr i64 %5 to i64*, !dbg !31
  call void @llvm.dbg.value(metadata i64* %6, metadata !20, metadata !DIExpression()), !dbg !31
  %7 = icmp eq i64* %dst, %6, !dbg !32
  call void @llvm.dbg.value(metadata i1 %7, metadata !21, metadata !DIExpression()), !dbg !32
  %8 = select i1 %7, i64* %dst, i64* %6, !dbg !33
  call void @llvm.dbg.value(metadata i64* %8, metadata !22, metadata !DIExpression()), !dbg !33
  %9 = ptrtoint i64* %8 to i64, !dbg !34
  call void @llvm.dbg.value(metadata i64 %9, metadata !23, metadata !DIExpression()), !dbg !34
  store i64 %9, i64* %dst, !dbg !35
  ret void, !dbg !36
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "preprocess.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_part64", linkageName: "test_part64", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL64_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CALL64_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IPTR_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[IPTR_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PTRI_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[PTRI_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i64, i64, i64*)* @test_part64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "preprocess.ll", directory: "/")
!5 = !{}
!6 = !{i32 13}
!7 = !{i32 9}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_part64", linkageName: "test_part64", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !16, !18, !19, !20, !21, !22, !23}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty96", size: 96, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !15)
!15 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !17)
!17 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !17)
!19 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 7, type: !17)
!20 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 8, type: !17)
!21 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 9, type: !15)
!22 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 10, type: !17)
!23 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 11, type: !17)
!24 = !DILocation(line: 1, column: 1, scope: !9)
!25 = !DILocation(line: 2, column: 1, scope: !9)
!26 = !DILocation(line: 3, column: 1, scope: !9)
!27 = !DILocation(line: 4, column: 1, scope: !9)
!28 = !DILocation(line: 5, column: 1, scope: !9)
!29 = !DILocation(line: 6, column: 1, scope: !9)
!30 = !DILocation(line: 7, column: 1, scope: !9)
!31 = !DILocation(line: 8, column: 1, scope: !9)
!32 = !DILocation(line: 9, column: 1, scope: !9)
!33 = !DILocation(line: 10, column: 1, scope: !9)
!34 = !DILocation(line: 11, column: 1, scope: !9)
!35 = !DILocation(line: 12, column: 1, scope: !9)
!36 = !DILocation(line: 13, column: 1, scope: !9)
