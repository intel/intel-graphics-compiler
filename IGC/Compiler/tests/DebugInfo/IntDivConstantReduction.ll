;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-intdiv-red -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction
; ------------------------------------------------
; This test checks that IntDivConstantReduction pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'IntDivConstantReduction.ll'
source_filename = "IntDivConstantReduction.ll"

define spir_kernel void @test_div(i32 %src1) !dbg !6 {
; Testcase 1
; Check that udiv debug value is preserved
; CHECK: [[UDIV_V:%[a-z0-9_]*]] = lshr i32 {{.*}} !dbg [[UDIV_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[UDIV_V]],  metadata [[UDIV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UDIV_LOC]]
  %1 = udiv i32 %src1, 3, !dbg !14
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !14
; Testcase 2
; Check that sdiv debug value is preserved
; CHECK: [[SDIV_V:%[a-z0-9_]*]] = sub i32 {{.*}} !dbg [[SDIV_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[SDIV_V]],  metadata [[SDIV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SDIV_LOC]]
  %2 = sdiv i32 %src1, -2147483648, !dbg !15
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !15
; Testcase 3
; Check that urem debug value is preserved
; CHECK: [[UREM_V:%[a-z0-9_]*]] = sub i32 {{.*}} !dbg [[UREM_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[UREM_V]],  metadata [[UREM_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UREM_LOC]]
  %3 = urem i32 %src1, 15, !dbg !16
  call void @llvm.dbg.value(metadata i32 %3, metadata !12, metadata !DIExpression()), !dbg !16
; Testcase 4
; Check that srem debug value is preserved
; CHECK: [[SREM_V:%[a-z0-9_]*]] = sub i32 {{.*}} !dbg [[SREM_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[SREM_V]],  metadata [[SREM_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SREM_LOC]]
  %4 = srem i32 %src1, 2, !dbg !17
  call void @llvm.dbg.value(metadata i32 %4, metadata !13, metadata !DIExpression()), !dbg !17
  ret void, !dbg !18
}


; Testcase 1 MD:
; CHECK-DAG: [[UDIV_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[UDIV_MD]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)

; Testcase 2 MD:
; CHECK-DAG: [[SDIV_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[SDIV_MD]] = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)

; Testcase 3 MD:
; CHECK-DAG: [[UREM_LOC]] = !DILocation(line: 3
; CHECK-DAG: [[UREM_MD]] = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)

; Testcase 4 MD:
; CHECK-DAG: [[SREM_LOC]] = !DILocation(line: 4
; CHECK-DAG: [[SREM_MD]] = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "IntDivConstantReduction.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_div", linkageName: "test_div", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocation(line: 1, column: 1, scope: !6)
!15 = !DILocation(line: 2, column: 1, scope: !6)
!16 = !DILocation(line: 3, column: 1, scope: !6)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
