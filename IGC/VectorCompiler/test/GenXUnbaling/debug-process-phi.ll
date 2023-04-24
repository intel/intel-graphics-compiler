;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXUnbalingWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXUnbaling
; ------------------------------------------------
; This test checks that GenXUnbaling pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.


; CHECK: define void @test_unbale{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i1> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; rdregion is moved to entry block
; CHECK-DAG: bb1:
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32>* [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL8_V:%[A-z0-9]*]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[VAL8_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL9_V:%[A-z0-9]*]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V]] = {{.*}}, !dbg [[VAL9_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i1> [[VAL10_V:%[A-z0-9]*]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC:![0-9]*]]
; CHECK-DAG: [[VAL10_V]] = {{.*}}, !dbg [[VAL10_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL11_V:%[A-z0-9]*]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC:![0-9]*]]
; CHECK-DAG: [[VAL11_V]] = {{.*}}, !dbg [[VAL11_LOC]]
; CHECK: end:
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32> [[VAL12_V:%[A-z0-9]*]], metadata [[VAL12_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL12_LOC:![0-9]*]]
; CHECK-DAG: [[VAL12_V]] = {{.*}}, !dbg [[VAL12_LOC]]

define void @test_unbale(<4 x i32>* %a) #0 !dbg !6 {
entry:
  %0 = load <4 x i32>, <4 x i32>* %a, !dbg !25
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !25
  %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %0, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !26
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !26
  %2 = icmp slt <4 x i32> %1, <i32 13, i32 15, i32 16, i32 17>, !dbg !27
  call void @llvm.dbg.value(metadata <4 x i1> %2, metadata !12, metadata !DIExpression()), !dbg !27
  %3 = extractelement <4 x i1> %2, i32 1, !dbg !28
  call void @llvm.dbg.value(metadata i1 %3, metadata !14, metadata !DIExpression()), !dbg !28
  br i1 %3, label %end, label %bb1, !dbg !29

bb1:                                              ; preds = %bb1, %entry
  %4 = phi <4 x i32> [ %0, %entry ], [ %7, %bb1 ], !dbg !30
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !16, metadata !DIExpression()), !dbg !30
  %5 = extractelement <4 x i32> %4, i32 2, !dbg !31
  call void @llvm.dbg.value(metadata i32 %5, metadata !17, metadata !DIExpression()), !dbg !31
  %6 = inttoptr i32 %5 to <4 x i32>*, !dbg !32
  call void @llvm.dbg.value(metadata <4 x i32>* %6, metadata !18, metadata !DIExpression()), !dbg !32
  %7 = load <4 x i32>, <4 x i32>* %6, !dbg !33
  call void @llvm.dbg.value(metadata <4 x i32> %7, metadata !20, metadata !DIExpression()), !dbg !33
  %8 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %0, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !34
  call void @llvm.dbg.value(metadata <4 x i32> %8, metadata !21, metadata !DIExpression()), !dbg !34
  %9 = icmp slt <4 x i32> %8, <i32 13, i32 15, i32 16, i32 17>, !dbg !35
  call void @llvm.dbg.value(metadata <4 x i1> %9, metadata !22, metadata !DIExpression()), !dbg !35
  %10 = extractelement <4 x i1> %9, i32 1, !dbg !36
  call void @llvm.dbg.value(metadata i1 %10, metadata !23, metadata !DIExpression()), !dbg !36
  br i1 %10, label %bb1, label %end, !dbg !37

end:                                              ; preds = %bb1, %entry
  %11 = phi <4 x i32> [ %0, %entry ], [ %4, %bb1 ], !dbg !38
  call void @llvm.dbg.value(metadata <4 x i32> %11, metadata !24, metadata !DIExpression()), !dbg !38
  store <4 x i32> %11, <4 x i32>* %a, !dbg !39
  ret void, !dbg !40
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "process-phi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_unbale", linkageName: "test_unbale", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL12_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL12_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "process-phi.ll", directory: "/")
!2 = !{}
!3 = !{i32 16}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_unbale", linkageName: "test_unbale", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !16, !17, !18, !20, !21, !22, !23, !24}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
!15 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !13)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !19)
!19 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 9, type: !10)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 10, type: !10)
!22 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 11, type: !13)
!23 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 12, type: !15)
!24 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 14, type: !10)
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
