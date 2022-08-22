;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-gen-specific-pattern -S < %s | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: select pattern optimization
; ------------------------------------------------
; This test checks that GenSpecificPattern pass follows
; 'How to Update Debug Info' llvm guideline.
;
; ------------------------------------------------

define spir_kernel void @test_select(i64 %src1, i64 %src2) !dbg !6 {
entry:
; Testcase1
; CHECK-DAG: [[LINE2:![0-9]+]] = !DILocation(line: 2
; CHECK-DAG: [[SELECT_I:![0-9]+]] = !DILocalVariable(name: "2"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[SELECT_IV:%[0-9]*]],  metadata [[SELECT_I]]
; CHECK-DAG: [[SELECT_IV]] = and i64 {{.*}}, !dbg [[LINE2]]
  %0 = icmp slt i64 %src1, %src2, !dbg !24
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !24
  %1 = select i1 %0, i64 %src2, i64 0, !dbg !25
  call void @llvm.dbg.value(metadata i64 %1, metadata !11, metadata !DIExpression()), !dbg !25

; Testcase2
; CHECK-DAG: [[LINE6:![0-9]+]] = !DILocation(line: 6
; CHECK-DAG: [[SELECT_F:![0-9]+]] = !DILocalVariable(name: "6"
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[SELECT_FV:%[0-9]*]],  metadata [[SELECT_F]]
; CHECK-DAG: [[SELECT_FV]] = bitcast {{.*}}, !dbg [[LINE6]]
  %2 = sitofp i64 %src1 to float, !dbg !26
  call void @llvm.dbg.value(metadata float %2, metadata !13, metadata !DIExpression()), !dbg !26
  %3 = sitofp i64 %src2 to float, !dbg !27
  call void @llvm.dbg.value(metadata float %3, metadata !15, metadata !DIExpression()), !dbg !27
  %4 = fcmp olt float %2, %3, !dbg !28
  call void @llvm.dbg.value(metadata i1 %4, metadata !16, metadata !DIExpression()), !dbg !28
  %5 = select i1 %4, float %3, float 0.000000e+00, !dbg !29
  call void @llvm.dbg.value(metadata float %5, metadata !17, metadata !DIExpression()), !dbg !29

; Testcase3
; CHECK-DAG: [[LINE8:![0-9]+]] = !DILocation(line: 8
; CHECK-DAG: [[SELECT_FC:![0-9]+]] = !DILocalVariable(name: "8"
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[SELECT_FCV:%[0-9]*]],  metadata [[SELECT_FC]]
; CHECK-DAG: [[SELECT_FCV]] = bitcast i32 {{.*}}, !dbg [[LINE8]]
  %fcmp = fcmp olt float %2, %3, !dbg !30
  call void @llvm.dbg.value(metadata i1 %fcmp, metadata !18, metadata !DIExpression()), !dbg !30
  %6 = select i1 %fcmp, float 3.000000e+00, float 0.000000e+00, !dbg !31
  call void @llvm.dbg.value(metadata float %6, metadata !19, metadata !DIExpression()), !dbg !31

; Testcase4
; CHECK-DAG: [[LINE12:![0-9]+]] = !DILocation(line: 12
; CHECK-DAG: [[SELECT_IC:![0-9]+]] = !DILocalVariable(name: "12"
; CHECK-DAG: [[DBG_VALUE_CALL]] i32 [[SELECT_ICV:%[0-9]*]],  metadata [[SELECT_IC]]
; CHECK-DAG: [[SELECT_ICV]] = trunc {{.*}}, !dbg [[LINE12]]
  %7 = sdiv i64 %src1, %src2, !dbg !32
  call void @llvm.dbg.value(metadata i64 %7, metadata !20, metadata !DIExpression()), !dbg !32
  %8 = trunc i64 %7 to i32, !dbg !33
  call void @llvm.dbg.value(metadata i32 %8, metadata !21, metadata !DIExpression()), !dbg !33
  %9 = icmp sgt i64 %7, 10, !dbg !34
  call void @llvm.dbg.value(metadata i1 %9, metadata !22, metadata !DIExpression()), !dbg !34
  %10 = select i1 %9, i32 %8, i32 10, !dbg !35
  call void @llvm.dbg.value(metadata i32 %10, metadata !23, metadata !DIExpression()), !dbg !35
  ret void, !dbg !36
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "SelectPattern.ll", directory: "/")
!2 = !{}
!3 = !{i32 13}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_select", linkageName: "test_select", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !16, !17, !18, !19, !20, !21, !22, !23}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !14)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !14)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !12)
!21 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !14)
!22 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !10)
!23 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !14)
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
