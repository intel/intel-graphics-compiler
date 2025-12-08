;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test generates DWARF for global variable.

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-15-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

@global = addrspace(1) global i64 0, align 8, !dbg !0
; CHECK: DW_TAG_variable
; CHECK-NEXT: DW_AT_name        : global
; CHECK-NEXT: DW_AT_type        : {{.*}}
; CHECK-NEXT: DW_AT_external    : {{.*}}
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}

; Function Attrs: nounwind
define spir_kernel void @foo() #0 !dbg !6 {
entry:
  ret void
}

attributes #0 = { nounwind readnone }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!9, !10}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "global", scope: !2, file: !3, line: 5, type: !5, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "clang version 14.0.5", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !4)
!3 = !DIFile(filename: "global.cpp", directory: "/tmp")
!4 = !{!0}
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = distinct !DISubprogram(name: "foo", scope: !3, file: !3, line: 3, type: !7, flags: DIFlagPrototyped, unit: !2, retainedNodes: !11)
!7 = !DISubroutineType(types: !8)
!8 = !{null}
!9 = !{i32 2, !"Dwarf Version", i32 4}
!10 = !{i32 1, !"Debug Info Version", i32 3}
!11 = !{}
