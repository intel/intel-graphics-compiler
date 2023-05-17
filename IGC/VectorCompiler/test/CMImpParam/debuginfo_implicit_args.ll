;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_LOCAL_ID

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_LOCAL_SIZE

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_GROUP_COUNT

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix CHECK_PRINT_BUFFER

declare <3 x i32> @llvm.genx.local.id.v3i32() #0
declare <3 x i32> @llvm.genx.local.size.v3i32() #0
declare <3 x i32> @llvm.genx.group.count.v3i32() #0
declare i64 @llvm.vc.internal.print.buffer() #0

define dllexport spir_kernel void @test_kernel() #0 !dbg !6 {
  %1 = call <3 x i32> @llvm.genx.local.id.v3i32()
  %2 = call <3 x i32> @llvm.genx.local.size.v3i32()
  %3 = call <3 x i32> @llvm.genx.group.count.v3i32()
  %4 = call i64 @llvm.vc.internal.print.buffer()
  ret void
}

; CHECK_LOCAL_ID:       @__imparg_llvm.genx.local.id16 = internal global <3 x i16> undef, !dbg ![[#EXPR_NODE:]]
; CHECK_LOCAL_ID:       @test_kernel
; CHECK_LOCAL_ID-DAG:   [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_LOCAL_ID-DAG:   [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_genx_local_id16", linkageName: "__llvm_genx_local_id16", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_LOCAL_ID-DAG:   [[#TYPE_NODE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: ![[#BASE_TYPE:]], size: 48, flags: DIFlagVector, elements: ![[#ELEMENTS:]])
; CHECK_LOCAL_ID-DAG:   [[#BASE_TYPE]] = !DIBasicType(name: "ui16", size: 16, encoding: DW_ATE_unsigned)
; CHECK_LOCAL_ID-DAG:   [[#ELEMENTS]] = !{![[#SUBRANGE:]]}
; CHECK_LOCAL_ID-DAG:   [[#SUBRANGE]] = !DISubrange(count: 3{{(, lowerBound: 0)?}})

; CHECK_LOCAL_SIZE:     @__imparg_llvm.genx.local.size = internal global <3 x i32> undef, !dbg ![[#EXPR_NODE:]]
; CHECK_LOCAL_SIZE:     @test_kernel
; CHECK_LOCAL_SIZE-DAG: [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_LOCAL_SIZE-DAG: [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_genx_local_size", linkageName: "__llvm_genx_local_size", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_LOCAL_SIZE-DAG: [[#TYPE_NODE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: ![[#BASE_TYPE:]], size: 96, flags: DIFlagVector, elements: ![[#ELEMENTS:]])
; CHECK_LOCAL_SIZE-DAG: [[#BASE_TYPE]] = !DIBasicType(name: "ui32", size: 32, encoding: DW_ATE_unsigned)
; CHECK_LOCAL_SIZE-DAG: [[#ELEMENTS]] = !{![[#SUBRANGE:]]}
; CHECK_LOCAL_SIZE-DAG: [[#SUBRANGE]] = !DISubrange(count: 3{{(, lowerBound: 0)?}})

; CHECK_GROUP_COUNT:     @__imparg_llvm.genx.group.count = internal global <3 x i32> undef, !dbg ![[#EXPR_NODE:]]
; CHECK_GROUP_COUNT:     @test_kernel
; CHECK_GROUP_COUNT-DAG: [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_GROUP_COUNT-DAG: [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_genx_group_count", linkageName: "__llvm_genx_group_count", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_GROUP_COUNT-DAG: [[#TYPE_NODE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: ![[#BASE_TYPE:]], size: 96, flags: DIFlagVector, elements: ![[#ELEMENTS:]])
; CHECK_GROUP_COUNT-DAG: [[#BASE_TYPE]] = !DIBasicType(name: "ui32", size: 32, encoding: DW_ATE_unsigned)
; CHECK_GROUP_COUNT-DAG: [[#ELEMENTS]] = !{![[#SUBRANGE:]]}
; CHECK_GROUP_COUNT-DAG: [[#SUBRANGE]] = !DISubrange(count: 3{{(, lowerBound: 0)?}})

; CHECK_PRINT_BUFFER:     @__imparg_llvm.vc.internal.print.buffer = internal global i64 undef, !dbg ![[#EXPR_NODE:]]
; CHECK_PRINT_BUFFER:     @test_kernel
; CHECK_PRINT_BUFFER-DAG: [[#EXPR_NODE]] = !DIGlobalVariableExpression(var: ![[#VAR_NODE:]], expr: !DIExpression())
; CHECK_PRINT_BUFFER-DAG: [[#VAR_NODE]]  = distinct !DIGlobalVariable(name: "__llvm_vc_internal_print_buffer", linkageName: "__llvm_vc_internal_print_buffer", scope: ![[#]], file: ![[#]], type: ![[#TYPE_NODE:]]
; CHECK_PRINT_BUFFER-DAG: [[#TYPE_NODE]] = !DIBasicType(name: "ui64", size: 64, encoding: DW_ATE_unsigned)

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
