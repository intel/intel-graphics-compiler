;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-handle-frem-inst -S < %s | FileCheck %s
; ------------------------------------------------
; HandleFRemInstructions
; ------------------------------------------------
; This test checks that HandleFRemInstructions pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'HandleFRemInstructions.ll'
source_filename = "HandleFRemInstructions.ll"

define spir_kernel void @test_frem(float %src1, float %src2) !dbg !6 {
; Testcase 1
; Check that scalar frem is substituted by call
; CHECK: [[FREM_V:%[0-9]*]] = call float @__builtin_spirv_OpFRem_f32_f32({{.*}} !dbg [[FREM_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] float [[FREM_V]],  metadata [[FREM_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FREM_LOC]]
  %1 = frem float %src1, %src2, !dbg !16
  call void @llvm.dbg.value(metadata float %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = insertelement <2 x float> zeroinitializer, float %1, i32 0, !dbg !17
  call void @llvm.dbg.value(metadata <2 x float> %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = insertelement <2 x float> zeroinitializer, float %src2, i32 0, !dbg !18
  call void @llvm.dbg.value(metadata <2 x float> %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = insertelement <2 x float> %2, float %src1, i32 1, !dbg !19
  call void @llvm.dbg.value(metadata <2 x float> %4, metadata !14, metadata !DIExpression()), !dbg !19
; Testcase 2
; Check that vector frem is substituted by call
; CHECK: [[FREM_VEC_V:%[0-9]*]] =  call <2 x float> @__builtin_spirv_OpFRem_v2f32_v2f32({{.*}} !dbg [[FREM_VEC_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <2 x float> [[FREM_VEC_V]],  metadata [[FREM_VEC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FREM_VEC_LOC]]
  %5 = frem <2 x float> %4, %2, !dbg !20
  call void @llvm.dbg.value(metadata <2 x float> %5, metadata !15, metadata !DIExpression()), !dbg !20
  ret void, !dbg !21
}

; Testcase 1 MD:
; CHECK-DAG: [[FREM_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[FREM_MD]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)

; Testcase 2 MD:
; CHECK-DAG: [[FREM_VEC_LOC]] = !DILocation(line: 5
; CHECK-DAG: [[FREM_VEC_MD]] = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "HandleFRemInstructions.ll", directory: "/")
!2 = !{}
!3 = !{i32 6}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_frem", linkageName: "test_frem", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
