;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test generates DWARF for global variable.

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

@global = addrspace(1) global i64 0, align 8, !dbg !0
; CHECK: DW_TAG_variable
; CHECK-NEXT: DW_AT_name        : global
; CHECK-NEXT: DW_AT_type        : {{.*}}
; CHECK-NEXT: DW_AT_external    : 1
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}

@linkage.__global = addrspace(1) global i32 0, align 4, !dbg !2
; CHECK: DW_TAG_variable
; CHECK-NEXT: DW_AT_name        : linked
; CHECK-NEXT: DW_AT_type        : {{.*}}
; CHECK-NEXT: DW_AT_external    : 1
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_linkage_name: linkage.__global

; Function Attrs: nounwind
define spir_kernel void @foo() #0 !dbg !8 {
entry:
  ret void
}

attributes #0 = { nounwind readnone }

!llvm.dbg.cu = !{!4}
!llvm.module.flags = !{!11, !12}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "global", scope: !4, file: !5, line: 5, type: !7, isLocal: false, isDefinition: true)
!2 = !DIGlobalVariableExpression(var: !3, expr: !DIExpression())
!3 = distinct !DIGlobalVariable(name: "linked", scope: !4, file: !5, line: 6, type: !7, isLocal: true, isDefinition: true)
!4 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !5, producer: "clang version 14.0.5", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !6)
!5 = !DIFile(filename: "global.cpp", directory: "/tmp")
!6 = !{!0, !2}
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!8 = distinct !DISubprogram(name: "foo", scope: !5, file: !5, line: 3, type: !9, flags: DIFlagPrototyped, unit: !4, retainedNodes: !13)
!9 = !DISubroutineType(types: !10)
!10 = !{null}
!11 = !{i32 2, !"Dwarf Version", i32 4}
!12 = !{i32 1, !"Debug Info Version", i32 3}
!13 = !{}
