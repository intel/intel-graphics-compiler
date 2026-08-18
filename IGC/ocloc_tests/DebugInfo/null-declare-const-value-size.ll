;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A null-pointer declare location is emitted as DW_AT_const_value, always 8
; bytes, which truncates any wider variable - here a 24-byte class - leaving the
; debugger to read garbage past the block. Assert on the mismatch, and emit no
; location at all, which reports the variable as optimized out.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus

; RUN: llvm-as %s -o %t

; The assert exits on some builds and aborts on others, so ignore how it died.
; RUN: %if debug %{ ocloc compile -llvm_input -file %t -device dg2 -options "-g" &> %t.log || true %}
; RUN: %if debug %{ FileCheck %s --check-prefix=ASSERT --input-file %t.log %}

; RUN: %if release && regkeys && oneapi-readelf %{ ocloc compile -llvm_input -file %t -device dg2 -options "-g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" %}
; RUN: %if release && regkeys && oneapi-readelf %{ oneapi-readelf --debug-dump=info %t_OCL_simd32_null_const_value.elf | FileCheck %s --check-prefix=DWARF %}

; ASSERT: {{[Aa]ssertion.*(DW_AT_const_value size does not match the variable type size|SizeMatches)}}

; The concrete DIE of the inlined "It" carries nothing but its abstract origin.
; DWARF:      DW_TAG_inlined_subroutine
; DWARF:      DW_TAG_formal_parameter
; DWARF-NEXT: DW_AT_abstract_origin
; DWARF-NEXT: Abbrev Number: 0

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @null_const_value() {
  call void @llvm.dbg.declare(metadata ptr null, metadata !4, metadata !DIExpression()), !dbg !16
  store i32 0, ptr addrspace(1) null, align 4, !dbg !23
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: false, emissionKind: FullDebug, enums: !3, imports: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!4 = !DILocalVariable(name: "It", arg: 2, scope: !5, file: !2, line: 87, type: !14)
!5 = distinct !DISubprogram(name: "inlinee", linkageName: "inlinee", scope: null, file: !2, line: 87, type: !11, scopeLine: 87, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !1, templateParams: !3, retainedNodes: !3)
!11 = distinct !DISubroutineType(types: !12)
!12 = !{null}
!14 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "item", file: !2, line: 35, size: 192, flags: DIFlagTypePassByValue, elements: !3, templateParams: !3)
!16 = !DILocation(line: 87, column: 30, scope: !5, inlinedAt: !17)
!17 = distinct !DILocation(line: 133, column: 5, scope: !21)
!21 = distinct !DISubprogram(name: "null_const_value", linkageName: "null_const_value", scope: null, file: !2, line: 83, type: !22, scopeLine: 131, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !1, templateParams: !3, retainedNodes: !3)
!22 = distinct !DISubroutineType(types: !12)
!23 = !DILocation(line: 89, column: 10, scope: !24, inlinedAt: !17)
!24 = distinct !DILexicalBlock(scope: !5, file: !2, line: 89, column: 364)
