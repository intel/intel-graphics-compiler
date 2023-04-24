;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLegalization
; ------------------------------------------------
; This test checks that GenXLegalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;


; CHECK: <4 x i1> @test_bitcast{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; unsalvageble vector
; CHECK: void @llvm.dbg.value({{.*}}, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i1> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i1> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]

define <4 x i1> @test_bitcast(i64* %a) !dbg !6 {
entry:
  %0 = load i64, i64* %a, !dbg !15
  call void @llvm.dbg.value(metadata i64 %0, metadata !9, metadata !DIExpression()), !dbg !15
  %1 = bitcast i64 %0 to <64 x i1>, !dbg !16
  call void @llvm.dbg.value(metadata <64 x i1> %1, metadata !11, metadata !DIExpression()), !dbg !16
  %2 = call <4 x i1> @llvm.genx.rdpredregion.v8i1.v64i1(<64 x i1> %1, i32 8), !dbg !17
  call void @llvm.dbg.value(metadata <4 x i1> %2, metadata !12, metadata !DIExpression()), !dbg !17
  %3 = xor <4 x i1> %2, <i1 true, i1 false, i1 true, i1 false>, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i1> %3, metadata !14, metadata !DIExpression()), !dbg !18
  ret <4 x i1> %3, !dbg !19
}

; CHECK: i64 @test_bitcastv{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <64 x i16> [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; unsalvageble vector
; CHECK: void @llvm.dbg.value({{.*}}, metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i64 [[VAL8_V:%[A-z0-9.]*]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}[[VAL8T_V:%[A-z0-9.]*]]{{.*}}, !dbg [[VAL8_LOC]]
; CHECK-DAG: [[VAL8T_V]] = {{.*}}, !dbg [[VAL8_LOC]]

define i64 @test_bitcastv(<32 x i32>* %a) !dbg !20 {
entry:
  %0 = load <32 x i32>, <32 x i32>* %a, !dbg !27
  call void @llvm.dbg.value(metadata <32 x i32> %0, metadata !22, metadata !DIExpression()), !dbg !27
  %1 = bitcast <32 x i32> %0 to <64 x i16>, !dbg !28
  call void @llvm.dbg.value(metadata <64 x i16> %1, metadata !24, metadata !DIExpression()), !dbg !28
  %2 = icmp slt <64 x i16> zeroinitializer, %1, !dbg !29
  call void @llvm.dbg.value(metadata <64 x i1> %2, metadata !25, metadata !DIExpression()), !dbg !29
  %3 = bitcast <64 x i1> %2 to i64, !dbg !30
  call void @llvm.dbg.value(metadata i64 %3, metadata !26, metadata !DIExpression()), !dbg !30
  ret i64 %3, !dbg !31
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "bitcast.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_bitcast", linkageName: "test_bitcast", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE1]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE1]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE1]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE1]])

; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_bitcastv", linkageName: "test_bitcastv", scope: null, file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE2]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE2]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE2]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE2]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE2]])

declare <4 x i1> @llvm.genx.rdpredregion.v8i1.v64i1(<64 x i1>, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "bitcast.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_bitcast", linkageName: "test_bitcast", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !13)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
!20 = distinct !DISubprogram(name: "test_bitcastv", linkageName: "test_bitcastv", scope: null, file: !1, line: 6, type: !7, scopeLine: 6, unit: !0, retainedNodes: !21)
!21 = !{!22, !24, !25, !26}
!22 = !DILocalVariable(name: "5", scope: !20, file: !1, line: 6, type: !23)
!23 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "6", scope: !20, file: !1, line: 7, type: !23)
!25 = !DILocalVariable(name: "7", scope: !20, file: !1, line: 8, type: !10)
!26 = !DILocalVariable(name: "8", scope: !20, file: !1, line: 9, type: !10)
!27 = !DILocation(line: 6, column: 1, scope: !20)
!28 = !DILocation(line: 7, column: 1, scope: !20)
!29 = !DILocation(line: 8, column: 1, scope: !20)
!30 = !DILocation(line: 9, column: 1, scope: !20)
!31 = !DILocation(line: 10, column: 1, scope: !20)
