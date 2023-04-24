;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXCFSimplification -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXCFSimplification
; ------------------------------------------------
; This test checks that GenXCFSimplification pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_cfsimplify{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i1> [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: [[VAL8_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; CHECK: [[VAL9_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
; CHECK: [[VAL10_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]
;
; other br are opt out
;
; CHECK: br {{.*}}, !dbg [[BR_LOC:![0-9]*]]
; CHECK: [[VAL11_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL11_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL11_V]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC]]

define void @test_cfsimplify(i32* %a) !dbg !6 {
entry:
  %0 = load i32, i32* %a, !dbg !24
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !24
  %1 = icmp ult i32 %0, 10, !dbg !25
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !25
  br i1 %1, label %end, label %bb1, !dbg !26

bb1:                                              ; preds = %entry
  %2 = add i32 %0, 1, !dbg !27
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !27
  %3 = getelementptr i32, i32* %a, i32 %2, !dbg !28
  call void @llvm.dbg.value(metadata i32* %3, metadata !14, metadata !DIExpression()), !dbg !28
  %4 = load i32, i32* %3, !dbg !29
  call void @llvm.dbg.value(metadata i32 %4, metadata !16, metadata !DIExpression()), !dbg !29
  %5 = bitcast i32 %4 to <32 x i1>, !dbg !30
  call void @llvm.dbg.value(metadata <32 x i1> %5, metadata !17, metadata !DIExpression()), !dbg !30
  %6 = call i1 @llvm.genx.any.v32i1(<32 x i1> %5), !dbg !31
  call void @llvm.dbg.value(metadata i1 %6, metadata !19, metadata !DIExpression()), !dbg !31
  br i1 %6, label %bb2, label %bb3, !dbg !32

bb2:                                              ; preds = %bb1
  br label %bb3, !dbg !33

bb3:                                              ; preds = %bb2, %bb1
  %7 = add i32 %4, 13, !dbg !34
  call void @llvm.dbg.value(metadata i32 %7, metadata !20, metadata !DIExpression()), !dbg !34
  %8 = select i1 %6, i32 %7, i32 %4, !dbg !35
  call void @llvm.dbg.value(metadata i32 %8, metadata !21, metadata !DIExpression()), !dbg !35
  %9 = shl i32 %8, 1, !dbg !36
  call void @llvm.dbg.value(metadata i32 %9, metadata !22, metadata !DIExpression()), !dbg !36
  br label %end, !dbg !37

end:                                              ; preds = %bb3, %entry
  %10 = phi i32 [ %0, %entry ], [ %9, %bb3 ], !dbg !38
  call void @llvm.dbg.value(metadata i32 %10, metadata !23, metadata !DIExpression()), !dbg !38
  store i32 %10, i32* %a, !dbg !39
  ret void, !dbg !40
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXCFSimplification.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_cfsimplify", linkageName: "test_cfsimplify", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])

declare i1 @llvm.genx.any.v32i1(<32 x i1>)

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXCFSimplification.ll", directory: "/")
!2 = !{}
!3 = !{i32 17}
!4 = !{i32 11}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_cfsimplify", linkageName: "test_cfsimplify", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !16, !17, !19, !20, !21, !22, !23}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !15)
!15 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !18)
!18 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !12)
!20 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 11, type: !10)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 12, type: !10)
!22 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 13, type: !10)
!23 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 15, type: !10)
!24 = !DILocation(line: 1, column: 1, scope: !6)
!25 = !DILocation(line: 2, column: 1, scope: !6)
!26 = !DILocation(line: 3, column: 1, scope: !6)
!27 = !DILocation(line: 4, column: 1, scope: !6)
!28 = !DILocation(line: 5, column: 1, scope: !6)
!29 = !DILocation(line: 6, column: 1, scope: !6)
!30 = !DILocation(line: 7, column: 1, scope: !6)
!31 = !DILocation(line: 8, column: 1, scope: !6)
!32 = !DILocation(line: 9, column: 1, scope: !6)
!33 = !DILocation(line: 10, column: 1, scope: !6)
!34 = !DILocation(line: 11, column: 1, scope: !6)
!35 = !DILocation(line: 12, column: 1, scope: !6)
!36 = !DILocation(line: 13, column: 1, scope: !6)
!37 = !DILocation(line: 14, column: 1, scope: !6)
!38 = !DILocation(line: 15, column: 1, scope: !6)
!39 = !DILocation(line: 16, column: 1, scope: !6)
!40 = !DILocation(line: 17, column: 1, scope: !6)
