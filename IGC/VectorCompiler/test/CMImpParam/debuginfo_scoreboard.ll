;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=true \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_SCOREBOARD_BTI

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=true \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_SCOREBOARD_DEPCNT

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=true \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_SCOREBOARD_DELTAS

declare i32 @llvm.genx.get.scoreboard.bti() #0
declare i32 @llvm.genx.get.scoreboard.depcnt() #0
declare <16 x i8> @llvm.genx.get.scoreboard.deltas() #0

define dllexport spir_kernel void @test_kernel() #0 !dbg !6 {
  %bti = call i32 @llvm.genx.get.scoreboard.bti()
  %depcnt = call i32 @llvm.genx.get.scoreboard.depcnt()
  %deltas = call <16 x i8> @llvm.genx.get.scoreboard.deltas()
  ret void
}

; CHECK_SCOREBOARD_BTI:     @__imparg_llvm.genx.get.scoreboard.bti = internal global i32 undef, !dbg ![[#EXPR_NODE:]]
; CHECK_SCOREBOARD_BTI:     @test_kernel
; CHECK_SCOREBOARD_BTI-DAG: [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_SCOREBOARD_BTI-DAG: [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_genx_get_scoreboard_bti", linkageName: "__llvm_genx_get_scoreboard_bti", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_SCOREBOARD_BTI-DAG: [[#TYPE_NODE]] = !DIBasicType(name: "ui32", size: 32, encoding: DW_ATE_unsigned)

; CHECK_SCOREBOARD_DEPCNT:     @__imparg_llvm.genx.get.scoreboard.depcnt = internal global i32 undef, !dbg ![[#EXPR_NODE:]]
; CHECK_SCOREBOARD_DEPCNT:     @test_kernel
; CHECK_SCOREBOARD_DEPCNT-DAG: [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_SCOREBOARD_DEPCNT-DAG: [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_genx_get_scoreboard_depcnt", linkageName: "__llvm_genx_get_scoreboard_depcnt", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_SCOREBOARD_DEPCNT-DAG: [[#TYPE_NODE]] = !DIBasicType(name: "ui32", size: 32, encoding: DW_ATE_unsigned)

; CHECK_SCOREBOARD_DELTAS: @__imparg_llvm.genx.get.scoreboard.deltas = internal global <16 x i8> undef, !dbg ![[#EXPR_NODE:]]
; CHECK_SCOREBOARD_DELTAS: @test_kernel
; CHECK_SCOREBOARD_DELTAS: [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_SCOREBOARD_DELTAS: [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_genx_get_scoreboard_deltas", linkageName: "__llvm_genx_get_scoreboard_deltas", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_SCOREBOARD_DELTAS: [[#TYPE_NODE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: ![[#BASE_TYPE:]], size: 128, flags: DIFlagVector, elements: ![[#ELEMENTS:]])
; CHECK_SCOREBOARD_DELTAS: [[#BASE_TYPE]] = !DIBasicType(name: "ui8", size: 8, encoding: DW_ATE_unsigned)
; CHECK_SCOREBOARD_DELTAS: [[#ELEMENTS]] = !{![[#SUBRANGE:]]}
; CHECK_SCOREBOARD_DELTAS: [[#SUBRANGE]] = !DISubrange(count: 16{{(, lowerBound: 0)?}})

attributes #0 = { "target-cpu"="Gen9" }

!genx.kernels = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!4}

!0 = !{void ()* @test_kernel, !"test_kernel", !1, i32 0, i32 0, !1, !1, i32 0}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "the_test.ll", directory: "/dev/null")
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !DILocation(line: 1, column: 1, scope: !6)
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !3, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !1)
!7 = !DISubroutineType(types: !1)

