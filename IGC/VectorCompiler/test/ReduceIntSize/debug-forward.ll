;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXReduceIntSize
; ------------------------------------------------
; This test checks that GenXReduceIntSize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_reduce{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i8 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; Note: dce doesn't set proper DIIExpr here
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK: store i32 {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL6_V:%[A-z0-9.]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK: store <4 x i32> {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i16> [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; correctly optimized out value(vectors not salvageble yet)
; CHECK: [[VAL9_V:%[A-z0-9.]*]] = add {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i16> [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
; correctly optimized out value(vectors not salvageble yet)
; Note: This value(from xor) is salvageble
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL11_V:%[A-z0-9.]*]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC:![0-9]*]]
; CHECK-DAG: [[VAL11_V]] = {{.*}}, !dbg [[VAL11_LOC]]
; CHECK: store <4 x i32> {{.*}} [[STORE3_LOC:![0-9]*]]


define void @test_reduce(i32* %a, <4 x i32>* %b, <4 x i16>* %c) !dbg !6 {
entry:
  %0 = load i32, i32* %a, !dbg !25
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !25
  %1 = trunc i32 %0 to i8, !dbg !26
  call void @llvm.dbg.value(metadata i8 %1, metadata !11, metadata !DIExpression()), !dbg !26
  %2 = zext i8 %1 to i32, !dbg !27
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !27
  %3 = add i32 %2, 14, !dbg !28
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !28
  store i32 %3, i32* %a, !dbg !29
  %4 = load <4 x i32>, <4 x i32>* %b, !dbg !30
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !15, metadata !DIExpression()), !dbg !30
  %5 = shl <4 x i32> %4, <i32 16, i32 16, i32 16, i32 16>, !dbg !31
  call void @llvm.dbg.value(metadata <4 x i32> %5, metadata !17, metadata !DIExpression()), !dbg !31
  %6 = lshr <4 x i32> %5, <i32 16, i32 16, i32 16, i32 16>, !dbg !32
  call void @llvm.dbg.value(metadata <4 x i32> %6, metadata !18, metadata !DIExpression()), !dbg !32
  store <4 x i32> %6, <4 x i32>* %b, !dbg !33
  %7 = load <4 x i16>, <4 x i16>* %c, !dbg !34
  call void @llvm.dbg.value(metadata <4 x i16> %7, metadata !19, metadata !DIExpression()), !dbg !34
  %8 = sext <4 x i16> %7 to <4 x i32>, !dbg !35
  call void @llvm.dbg.value(metadata <4 x i32> %8, metadata !21, metadata !DIExpression()), !dbg !35
  %9 = add <4 x i16> %7, <i16 3, i16 4, i16 5, i16 6>, !dbg !36
  call void @llvm.dbg.value(metadata <4 x i16> %9, metadata !22, metadata !DIExpression()), !dbg !36
  %10 = sext <4 x i16> %9 to <4 x i32>, !dbg !37
  call void @llvm.dbg.value(metadata <4 x i32> %10, metadata !23, metadata !DIExpression()), !dbg !37
  %11 = xor <4 x i32> %10, %8, !dbg !38
  call void @llvm.dbg.value(metadata <4 x i32> %11, metadata !24, metadata !DIExpression()), !dbg !38
  store <4 x i32> %11, <4 x i32>* %b, !dbg !39
  ret void, !dbg !40
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "forward.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_reduce", linkageName: "test_reduce", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "forward.ll", directory: "/")
!2 = !{}
!3 = !{i32 16}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_reduce", linkageName: "test_reduce", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !17, !18, !19, !21, !22, !23, !24}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !16)
!16 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !16)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !16)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 10, type: !20)
!20 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 11, type: !16)
!22 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 12, type: !20)
!23 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 13, type: !16)
!24 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 14, type: !16)
!25 = !DILocation(line: 1, column: 1, scope: !6)
!26 = !DILocation(line: 2, column: 1, scope: !6)
!27 = !DILocation(line: 3, column: 1, scope: !6)
!28 = !DILocation(line: 4, column: 1, scope: !6)
!29 = !DILocation(line: 5, column: 1, scope: !6)
!30 = !DILocation(line: 6, column: 1, scope: !6)
!31 = !DILocation(line: 7, column: 1, scope: !6)
!32 = !DILocation(line: 8, column: 1, scope: !6)
!33 = !DILocation(line: 9, column: 1, scope: !6)
!34 = !DILocation(line: 10, column: 1, scope: !6)
!35 = !DILocation(line: 11, column: 1, scope: !6)
!36 = !DILocation(line: 12, column: 1, scope: !6)
!37 = !DILocation(line: 13, column: 1, scope: !6)
!38 = !DILocation(line: 14, column: 1, scope: !6)
!39 = !DILocation(line: 15, column: 1, scope: !6)
!40 = !DILocation(line: 16, column: 1, scope: !6)
