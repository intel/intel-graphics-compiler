;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -breakdown-intrinsics -S < %s | FileCheck %s
; ------------------------------------------------
; BreakdownIntrinsic
; ------------------------------------------------
; This test checks that BreakdownIntrinsic pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'BreakdownIntrinsicPass.ll'
source_filename = "BreakdownIntrinsicPass.ll"

define spir_kernel void @test_breakintr(float %a, float %b, float %c) !dbg !9 {
;
; Testcase 1:
;
; fmuladd -> fmul + fadd
;
; CHECK: [[FMUL_V:%[0-9]*]] = fmul
; CHECK-NEXT: [[FMULADD_V:%[0-9]*]] = fadd {{.*}} [[FMUL_V]]{{.*}} !dbg [[FMULADD_LOC:![0-9]*]]
; CHECK-NEXT: dbg.value(metadata float [[FMULADD_V]], metadata [[FMULADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FMULADD_LOC]]
;
  %1 = call fast float @llvm.fmuladd.f32(float %a, float %b, float %c), !dbg !15
  call void @llvm.dbg.value(metadata float %1, metadata !12, metadata !DIExpression()), !dbg !15
;
; Testcase 2:
;
; fma -> fmul + fadd
;
; CHECK: [[FMUL_V:%[0-9]*]] = fmul
; CHECK-NEXT: [[FMA_V:%[0-9]*]] = fadd {{.*}} [[FMUL_V]]{{.*}} !dbg [[FMA_LOC:![0-9]*]]
; CHECK-NEXT: dbg.value(metadata float [[FMA_V]], metadata [[FMA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FMA_LOC]]
;
  %2 = call fast float @llvm.fma.f32(float %a, float %b, float %c), !dbg !16
  call void @llvm.dbg.value(metadata float %2, metadata !14, metadata !DIExpression()), !dbg !16
  ret void, !dbg !17
}

; Testcase 1 MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "BreakdownIntrinsicPass.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_breakintr",
; CHECK-DAG: [[I32_TY:![0-9]*]] = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
;
; CHECK-DAG: [[FMULADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FMULADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1, type: [[I32_TY]])

; Testcase 2 MD:
; CHECK-DAG: [[FMA_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FMA_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2, type: [[I32_TY]])


; Function Attrs: nounwind readnone speculatable
declare float @llvm.fma.f32(float, float, float) #0

; Function Attrs: nounwind readnone speculatable
declare float @llvm.fmuladd.f32(float, float, float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"UnsafeMathOptimizations", i1 true}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "BreakdownIntrinsicPass.ll", directory: "/")
!5 = !{}
!6 = !{i32 3}
!7 = !{i32 2}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_breakintr", linkageName: "test_breakintr", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocation(line: 1, column: 1, scope: !9)
!16 = !DILocation(line: 2, column: 1, scope: !9)
!17 = !DILocation(line: 3, column: 1, scope: !9)
