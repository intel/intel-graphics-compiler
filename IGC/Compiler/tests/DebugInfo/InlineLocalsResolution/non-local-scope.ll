;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-resolve-inline-locals -S < %s | FileCheck %s --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-DBG-RECORDS%} %else %{CHECK-DBG-INTRINSIC%}
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------
; This test checks if Utils::updateGlobalVarDebugInfo correctly re-parents the debug info
; of a global variable whose DIGlobalVariable scope is DIModule, as it's something that
; Fortran's Front End produces.
; ------------------------------------------------

; CHECK: void @test_kernel{{.*}}!dbg [[SCOPE:![0-9]*]]
; CHECK-DBG-INTRINSIC: dbg.declare(metadata ptr addrspace(3) @local_arr, metadata [[VAR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOC:![0-9]*]]
; CHECK-DBG-RECORDS: #dbg_declare(ptr addrspace(3) @local_arr, [[VAR_MD:![0-9]*]], !DIExpression(), [[LOC:![0-9]*]])

; The localized variable must be scoped to the sub-program, not to the !DIModule.
; CHECK-DAG: [[VAR_MD]] = !DILocalVariable(name: "local_arr", scope: [[SCOPE]], {{.*}}line: 10
; CHECK-DAG: [[LOC]] = !DILocation(line: 21, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel"

@local_arr = addrspace(3) global [64 x i64] zeroinitializer, align 32, !dbg !0

define spir_kernel void @test_kernel(ptr %dst) !dbg !11 {
  %1 = ptrtoint ptr addrspace(3) @local_arr to i64, !dbg !14
  store i64 %1, ptr %dst, !dbg !15
  ret void, !dbg !16
}

!IGCMetadata = !{!17}
!igc.functions = !{!20}
!llvm.dbg.cu = !{!8}
!llvm.module.flags = !{!13}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "local_arr", linkageName: "local_test_module_mp_local_arr_", scope: !2, file: !3, line: 10, type: !4, isLocal: false, isDefinition: true)
!2 = !DIModule(scope: null, name: "local_test_module", file: !3, line: 7)
!3 = !DIFile(filename: "test.f90", directory: "/")
!4 = !DICompositeType(tag: DW_TAG_array_type, baseType: !5, size: 4096, elements: !6)
!5 = !DIBasicType(name: "INTEGER*8", size: 64, encoding: DW_ATE_signed)
!6 = !{!7}
!7 = !DISubrange(count: 64, lowerBound: 1)
!8 = distinct !DICompileUnit(language: DW_LANG_Fortran95, file: !3, producer: "ifx", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !10, globals: !9)
!9 = !{!0}
!10 = !{}
!11 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: !3, file: !3, line: 20, type: !12, scopeLine: 20, unit: !8, retainedNodes: !10)
!12 = !DISubroutineType(types: !10)
!13 = !{i32 2, !"Debug Info Version", i32 3}
!14 = !DILocation(line: 21, column: 1, scope: !11)
!15 = !DILocation(line: 22, column: 1, scope: !11)
!16 = !DILocation(line: 23, column: 1, scope: !11)
!17 = !{!"ModuleMD", !18}
!18 = !{!"compOpt", !19}
!19 = !{!"OptDisable", i1 false}
!20 = !{ptr @test_kernel, !21}
!21 = !{!22}
!22 = !{!"function_type", i32 0}
