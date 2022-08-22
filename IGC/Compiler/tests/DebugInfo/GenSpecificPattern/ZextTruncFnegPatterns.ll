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
; GenSpecificPattern: zext, trunc and fneg patterns
; ------------------------------------------------
; This test checks that GenSpecificPattern pass follows
; 'How to Update Debug Info' llvm guideline.
;
; ------------------------------------------------

define spir_kernel void @test_zext(i32 %src1, i64 %src2, float %src3) !dbg !6 {
entry:
; Testcase1:
; CHECK-DAG: [[LINE2:![0-9]+]] = !DILocation(line: 2
; CHECK-DAG: [[ZEXT_MD:![0-9]+]] = !DILocalVariable(name: "2"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[ZEXT_V:%[0-9]*]],  metadata [[ZEXT_MD]]
; CHECK-DAG: [[ZEXT_V]] = sub i32 0, {{.*}}, !dbg [[LINE2]]
  %0 = icmp slt i32 %src1, 2, !dbg !22
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !22
  %1 = zext i1 %0 to i32, !dbg !23
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !23

; Testcase2:
; CHECK-DAG: [[LINE4:![0-9]+]] = !DILocation(line: 4
; CHECK-DAG: [[I2PTR_MD:![0-9]+]] = !DILocalVariable(name: "4"
; CHECK-DAG: [[DBG_VALUE_CALL]] i64* [[I2PTR_V:%[0-9]*]],  metadata [[I2PTR_MD]]
; CHECK-DAG: [[I2PTR_V]] = inttoptr i32 {{.*}}, !dbg [[LINE4]]
  %2 = zext i32 %src1 to i64, !dbg !24
  call void @llvm.dbg.value(metadata i64 %2, metadata !13, metadata !DIExpression()), !dbg !24
  %a = inttoptr i64 %2 to i64*, !dbg !25
  call void @llvm.dbg.value(metadata i64* %a, metadata !15, metadata !DIExpression()), !dbg !25

; Testcase3:
; CHECK-DAG: [[LINE6:![0-9]+]] = !DILocation(line: 6
; CHECK-DAG: [[TRUNC_MD:![0-9]+]] = !DILocalVariable(name: "6"
; CHECK-DAG: [[DBG_VALUE_CALL]] i32 [[TRUNC_V:%[0-9]*]],  metadata [[TRUNC_MD]]
; CHECK-DAG: [[TRUNC_V]] = lshr i32 {{.*}}, 17, !dbg [[LINE6]]
  %3 = lshr i64 %src2, 49, !dbg !26
  call void @llvm.dbg.value(metadata i64 %3, metadata !16, metadata !DIExpression()), !dbg !26
  %4 = trunc i64 %3 to i32, !dbg !27
  call void @llvm.dbg.value(metadata i32 %4, metadata !17, metadata !DIExpression()), !dbg !27

; Testcase4:
; CHECK-DAG: [[LINE7:![0-9]+]] = !DILocation(line: 7
; CHECK-DAG: [[FNEG_MD:![0-9]+]] = !DILocalVariable(name: "7"
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[FNEG_V:%[0-9]*]],  metadata [[FNEG_MD]]
; CHECK-DAG: [[FNEG_V]] = {{.*}}, !dbg [[LINE7]]
  %5 = fneg float %src3, !dbg !28
  call void @llvm.dbg.value(metadata float %5, metadata !18, metadata !DIExpression()), !dbg !28

; Testcase5:
; CHECK-DAG: [[LINE10:![0-9]+]] = !DILocation(line: 10
; CHECK-DAG: [[FNEG_MD:![0-9]+]] = !DILocalVariable(name: "10"
; CHECK-DAG: [[DBG_VALUE_CALL]] <2 x float> [[FNEG_V:%[0-9]*]],  metadata [[FNEG_MD]]
; CHECK-DAG: [[FNEG_V]] = {{.*}}, !dbg [[LINE10]]
  %6 = insertelement <2 x float> undef, float %src3, i32 0, !dbg !29
  call void @llvm.dbg.value(metadata <2 x float> %6, metadata !19, metadata !DIExpression()), !dbg !29
  %7 = insertelement <2 x float> %6, float 0.000000e+00, i32 1, !dbg !30
  call void @llvm.dbg.value(metadata <2 x float> %7, metadata !20, metadata !DIExpression()), !dbg !30
  %8 = fneg <2 x float> %7, !dbg !31
  call void @llvm.dbg.value(metadata <2 x float> %8, metadata !21, metadata !DIExpression()), !dbg !31
  ret void, !dbg !32
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ZextTruncFnegPatterns.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 10}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_zext", linkageName: "test_zext", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !16, !17, !18, !19, !20, !21}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !14)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !12)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !14)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !14)
!21 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !14)
!22 = !DILocation(line: 1, column: 1, scope: !6)
!23 = !DILocation(line: 2, column: 1, scope: !6)
!24 = !DILocation(line: 3, column: 1, scope: !6)
!25 = !DILocation(line: 4, column: 1, scope: !6)
!26 = !DILocation(line: 5, column: 1, scope: !6)
!27 = !DILocation(line: 6, column: 1, scope: !6)
!28 = !DILocation(line: 7, column: 1, scope: !6)
!29 = !DILocation(line: 8, column: 1, scope: !6)
!30 = !DILocation(line: 9, column: 1, scope: !6)
!31 = !DILocation(line: 10, column: 1, scope: !6)
!32 = !DILocation(line: 11, column: 1, scope: !6)
