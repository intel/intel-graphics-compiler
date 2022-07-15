;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-emu64ops -S < %s | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------
; This test checks that Emu64Ops pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_emu64{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: = {{.*}}, !dbg [[ADD_LOC:![0-9]*]]
; Value is undef here, check DIExpr before filing
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: = {{.*}}, !dbg [[SUB_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[SUB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SUB_LOC]]
; CHECK: = {{.*}}, !dbg [[MUL_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[MUL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[MUL_LOC]]
; CHECK: = {{.*}}, !dbg [[AND_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[AND_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AND_LOC]]
; CHECK: = {{.*}}, !dbg [[OR_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[OR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[OR_LOC]]
; CHECK: = {{.*}}, !dbg [[XOR_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[XOR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[XOR_LOC]]
; CHECK-DAG: llvm.dbg.value(metadata i1 [[ICMP_V:%[A-z0-9]*]], metadata [[ICMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP_LOC:![0-9]*]]
; CHECK-DAG: [[ICMP_V]] = {{.*}}, !dbg [[ICMP_LOC]]
; CHECK: = {{.*}}, !dbg [[SELECT_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[SELECT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT_LOC]]
; CHECK: = {{.*}}, !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: [[BCAST_V:%[A-z0-9]*]] = bitcast {{.*}}, !dbg [[BCAST_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC]]
; CHECK: llvm.dbg.value
; CHECK: [[TRUNC_V:%[A-z0-9]*]] = trunc {{.*}}, !dbg [[TRUNC_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[TRUNC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TRUNC_LOC]]
; CHECK: llvm.dbg.value
; CHECK: = {{.*}}, !dbg [[SEXT_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[SEXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SEXT_LOC]]
; CHECK: store{{.*}} !dbg [[STORE_LOC:![0-9]*]]

define void @test_emu64(i64 %a, i64 %b, i64* %dst) !dbg !9 {
  %1 = add i64 %a, %b, !dbg !29
  call void @llvm.dbg.value(metadata i64 %1, metadata !12, metadata !DIExpression()), !dbg !29
  %2 = sub i64 %a, 13, !dbg !30
  call void @llvm.dbg.value(metadata i64 %2, metadata !14, metadata !DIExpression()), !dbg !30
  %3 = mul i64 %1, %2, !dbg !31
  call void @llvm.dbg.value(metadata i64 %3, metadata !15, metadata !DIExpression()), !dbg !31
  %4 = and i64 %3, %1, !dbg !32
  call void @llvm.dbg.value(metadata i64 %4, metadata !16, metadata !DIExpression()), !dbg !32
  %5 = or i64 %4, %a, !dbg !33
  call void @llvm.dbg.value(metadata i64 %5, metadata !17, metadata !DIExpression()), !dbg !33
  %6 = xor i64 %5, %b, !dbg !34
  call void @llvm.dbg.value(metadata i64 %6, metadata !18, metadata !DIExpression()), !dbg !34
  %7 = icmp sge i64 %6, %a, !dbg !35
  call void @llvm.dbg.value(metadata i1 %7, metadata !19, metadata !DIExpression()), !dbg !35
  %8 = select i1 %7, i64 %a, i64 %b, !dbg !36
  call void @llvm.dbg.value(metadata i64 %8, metadata !21, metadata !DIExpression()), !dbg !36
  %9 = load i64, i64* %dst, align 4, !dbg !37
  call void @llvm.dbg.value(metadata i64 %9, metadata !22, metadata !DIExpression()), !dbg !37
  %10 = bitcast i64 %9 to <4 x i16>, !dbg !38
  call void @llvm.dbg.value(metadata <4 x i16> %10, metadata !23, metadata !DIExpression()), !dbg !38
  %11 = extractelement <4 x i16> %10, i32 2, !dbg !39
  call void @llvm.dbg.value(metadata i16 %11, metadata !24, metadata !DIExpression()), !dbg !39
  %12 = trunc i64 %8 to i16, !dbg !40
  call void @llvm.dbg.value(metadata i16 %12, metadata !26, metadata !DIExpression()), !dbg !40
  %13 = add i16 %12, %11, !dbg !41
  call void @llvm.dbg.value(metadata i16 %13, metadata !27, metadata !DIExpression()), !dbg !41
  %14 = sext i16 %13 to i64, !dbg !42
  call void @llvm.dbg.value(metadata i64 %14, metadata !28, metadata !DIExpression()), !dbg !42
  store i64 %14, i64* %dst, align 4, !dbg !43
  ret void, !dbg !44
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_emu64", linkageName: "test_emu64", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SUB_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[SUB_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[MUL_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[MUL_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[AND_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[AND_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[OR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[OR_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[XOR_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[XOR_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ICMP_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[ICMP_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[SELECT_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[TRUNC_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[TRUNC_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SEXT_MD]] = !DILocalVariable(name: "14", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[SEXT_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i64, i64, i64*)* @test_emu64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "basic.ll", directory: "/")
!5 = !{}
!6 = !{i32 16}
!7 = !{i32 14}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_emu64", linkageName: "test_emu64", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16, !17, !18, !19, !21, !22, !23, !24, !26, !27, !28}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !13)
!17 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !13)
!18 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 6, type: !13)
!19 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 7, type: !20)
!20 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 8, type: !13)
!22 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 9, type: !13)
!23 = !DILocalVariable(name: "10", scope: !9, file: !4, line: 10, type: !13)
!24 = !DILocalVariable(name: "11", scope: !9, file: !4, line: 11, type: !25)
!25 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!26 = !DILocalVariable(name: "12", scope: !9, file: !4, line: 12, type: !25)
!27 = !DILocalVariable(name: "13", scope: !9, file: !4, line: 13, type: !25)
!28 = !DILocalVariable(name: "14", scope: !9, file: !4, line: 14, type: !13)
!29 = !DILocation(line: 1, column: 1, scope: !9)
!30 = !DILocation(line: 2, column: 1, scope: !9)
!31 = !DILocation(line: 3, column: 1, scope: !9)
!32 = !DILocation(line: 4, column: 1, scope: !9)
!33 = !DILocation(line: 5, column: 1, scope: !9)
!34 = !DILocation(line: 6, column: 1, scope: !9)
!35 = !DILocation(line: 7, column: 1, scope: !9)
!36 = !DILocation(line: 8, column: 1, scope: !9)
!37 = !DILocation(line: 9, column: 1, scope: !9)
!38 = !DILocation(line: 10, column: 1, scope: !9)
!39 = !DILocation(line: 11, column: 1, scope: !9)
!40 = !DILocation(line: 12, column: 1, scope: !9)
!41 = !DILocation(line: 13, column: 1, scope: !9)
!42 = !DILocation(line: 14, column: 1, scope: !9)
!43 = !DILocation(line: 15, column: 1, scope: !9)
!44 = !DILocation(line: 16, column: 1, scope: !9)
