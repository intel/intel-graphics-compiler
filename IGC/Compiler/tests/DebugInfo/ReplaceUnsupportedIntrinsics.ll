;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-replace-unsupported-intrinsics -S < %s | FileCheck %s
; ------------------------------------------------
; ReplaceUnsupportedIntrinsics: funnel shifts
; ------------------------------------------------
; This test checks that ReplaceUnsupportedIntrinsics pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'ReplaceUnsupportedIntrinsics.ll'
source_filename = "ReplaceUnsupportedIntrinsics.ll"

define spir_kernel void @test_shift(i32 %src1, i32 %src2, i32 %src3) !dbg !6 {
entry:
; Testcase 1:
; CHECK-LABEL @test_shift
; CHECK: [[UREM:%[0-9]*]] = and i32 %src3, 31
; CHECK-NEXT: [[SUB:%[0-9]*]] = sub i32 32, [[UREM]]
; CHECK-NEXT: [[SHL:%[0-9]*]] = shl i32 %src1, [[UREM]]
; CHECK-NEXT: [[LSHR:%[0-9]*]] = lshr i32 %src2, [[SUB]]
; CHECK-NEXT: [[FSHL_V:%[0-9]*]] = or i32 [[SHL]], [[LSHR]], !dbg [[FSHL_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[FSHL_V]],  metadata [[FSHL_MD:![0-9]*]], {{.*}} !dbg [[FSHL_LOC]]
  %fshl_result = call i32 @llvm.fshl.i32(i32 %src1, i32 %src2, i32 %src3), !dbg !20
  call void @llvm.dbg.value(metadata i32 %fshl_result, metadata !9, metadata !DIExpression()), !dbg !20

; Testcase 2:
; CHECK-NEXT: [[UREM:%[0-9]*]] = and i32 %src3, 31
; CHECK-NEXT: [[SUB:%[0-9]*]] = sub i32 32, [[UREM]]
; CHECK-NEXT: [[SHL:%[0-9]*]] = shl i32 %src1, [[SUB]]
; CHECK-NEXT: [[LSHR:%[0-9]*]] = lshr i32 %src2, [[UREM]]
; CHECK-NEXT: [[FSHR_V:%[0-9]*]] = or i32 [[SHL]], [[LSHR]], !dbg [[FSHR_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[FSHR_V]],  metadata [[FSHR_MD:![0-9]*]], {{.*}} !dbg [[FSHR_LOC]]
  %fshr_result = call i32 @llvm.fshr.i32(i32 %src1, i32 %src2, i32 %src3), !dbg !21
  call void @llvm.dbg.value(metadata i32 %fshr_result, metadata !11, metadata !DIExpression()), !dbg !21
; Testcase 3:
; prepare vectors
  %0 = insertelement <2 x i32> undef, i32 %src1, i32 0, !dbg !22
  call void @llvm.dbg.value(metadata <2 x i32> %0, metadata !12, metadata !DIExpression()), !dbg !22
  %1 = insertelement <2 x i32> %0, i32 %src2, i32 1, !dbg !23
  call void @llvm.dbg.value(metadata <2 x i32> %1, metadata !14, metadata !DIExpression()), !dbg !23
  %2 = insertelement <2 x i32> undef, i32 %src2, i32 0, !dbg !24
  call void @llvm.dbg.value(metadata <2 x i32> %2, metadata !15, metadata !DIExpression()), !dbg !24
  %3 = insertelement <2 x i32> %2, i32 %src3, i32 1, !dbg !25
  call void @llvm.dbg.value(metadata <2 x i32> %3, metadata !16, metadata !DIExpression()), !dbg !25
  %4 = insertelement <2 x i32> undef, i32 %src3, i32 0, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i32> %4, metadata !17, metadata !DIExpression()), !dbg !26
  %5 = insertelement <2 x i32> %4, i32 %src1, i32 1, !dbg !27
  call void @llvm.dbg.value(metadata <2 x i32> %5, metadata !18, metadata !DIExpression()), !dbg !27
; fshr call
; CHECK: [[UREM:%[0-9]*]] = and <2 x i32> {{.*}}, <i32 31, i32 31>
; CHECK-NEXT: [[SUB:%[0-9]*]] = sub <2 x i32> <i32 32, i32 32>, [[UREM]]
; CHECK-NEXT: [[SHL:%[0-9]*]] = shl <2 x i32> {{.*}}, [[SUB]]
; CHECK-NEXT: [[LSHR:%[0-9]*]] = lshr <2 x i32> {{.*}}, [[UREM]]
; CHECK-NEXT: [[FSHRV_V:%[0-9]*]] = or <2 x i32> [[SHL]], [[LSHR]], !dbg [[FSHRV_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <2 x i32> [[FSHRV_V]],  metadata [[FSHRV_MD:![0-9]*]], {{.*}} !dbg [[FSHRV_LOC]]
  %fshr_vector_result = call <2 x i32> @llvm.fshr.v2i32(<2 x i32> %1, <2 x i32> %3, <2 x i32> %5), !dbg !28
  call void @llvm.dbg.value(metadata <2 x i32> %fshr_vector_result, metadata !19, metadata !DIExpression()), !dbg !28
  ret void, !dbg !29
}
; Testcase 1 MD:
; CHECK-DAG: [[FSHL_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[FSHL_MD]] = !DILocalVariable(name: "1"

; Testcase 2 MD:
; CHECK-DAG: [[FSHR_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[FSHR_MD]] = !DILocalVariable(name: "2"

; Testcase 3 MD:
; CHECK-DAG: [[FSHRV_LOC]] = !DILocation(line: 9
; CHECK-DAG: [[FSHRV_MD]] = !DILocalVariable(name: "9"

; Function Attrs: nounwind readnone speculatable
declare i32 @llvm.fshl.i32(i32, i32, i32) #0

; Function Attrs: nounwind readnone speculatable
declare i32 @llvm.fshr.i32(i32, i32, i32) #0

; Function Attrs: nounwind readnone speculatable
declare <2 x i32> @llvm.fshr.v2i32(<2 x i32>, <2 x i32>, <2 x i32>) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ReplaceUnsupportedIntrinsics.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_shift", linkageName: "test_shift", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !16, !17, !18, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !13)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !13)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !13)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !13)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !13)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !13)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)
!28 = !DILocation(line: 9, column: 1, scope: !6)
!29 = !DILocation(line: 10, column: 1, scope: !6)
