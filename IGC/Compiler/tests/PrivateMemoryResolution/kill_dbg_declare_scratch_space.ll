;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; On BMG an alloca is lowered to an offset into the scratch surface, which the debugger
; cannot dereference. Its dbg.declare location must be killed.

; REQUIRES: llvm-16-plus

; RUN: igc_opt --opaque-pointers --igc-private-mem-resolution --platformbmg -S %s | FileCheck %s --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-DBG-RECORDS%} %else %{CHECK-DBG-INTRINSIC%}

; Alloca sinking places the lowering after a dbg.declare intrinsic but before a debug record.
; CHECK-DAG: inttoptr i32 %{{.*}} to ptr
; CHECK-DBG-INTRINSIC-DAG: call void @llvm.dbg.declare(metadata ptr poison, metadata ![[VAR:[0-9]+]], metadata !DIExpression()), !dbg !{{[0-9]+}}
; CHECK-DBG-RECORDS-DAG: #dbg_declare(ptr poison, ![[VAR:[0-9]+]], !DIExpression(), !{{[0-9]+}})
; CHECK: ![[VAR]] = !DILocalVariable(name: "arr"

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(ptr addrspace(1) %out, i64 %idx) {
  %arr = alloca [64 x i32], align 4
  call void @llvm.dbg.declare(metadata ptr %arr, metadata !10, metadata !DIExpression()), !dbg !15
  %p = getelementptr [64 x i32], ptr %arr, i64 0, i64 %idx
  store i32 1, ptr %p, align 4
  %v = load i32, ptr %arr, align 4
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}
!igc.functions = !{!3}
!IGCMetadata = !{!19}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!2 = !DIFile(filename: "test.cpp", directory: "/")
!3 = !{ptr @foo, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!10 = !DILocalVariable(name: "arr", scope: !11, file: !2, line: 3, type: !12)
!11 = distinct !DISubprogram(name: "foo", scope: null, file: !2, line: 1, type: !13, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !1)
!12 = !DICompositeType(tag: DW_TAG_array_type, baseType: !16, size: 2048, elements: !17)
!13 = !DISubroutineType(types: !14)
!14 = !{null}
!15 = !DILocation(line: 3, column: 7, scope: !11)
!16 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!17 = !{!18}
!18 = !DISubrange(count: 64)
!19 = !{!"ModuleMD", !20}
!20 = !{!"compOpt", !21}
!21 = !{!"OptDisable", i1 false}
