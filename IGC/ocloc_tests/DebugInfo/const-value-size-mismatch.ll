;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; An 8-byte constant extracts to 8 bytes of DW_AT_const_value, truncating a wider
; variable - here a 24-byte class - so assert and emit no location instead.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus, regkeys

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t

; The assert exits on some builds and aborts on others, so ignore how it died.
; RUN: %if debug %{ ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1'" &> %t.log || true %}
; RUN: %if debug %{ FileCheck %s --check-prefix=ASSERT --input-file %t.log %}

; RUN: %if release && regkeys && oneapi-readelf %{ ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" %}
; RUN: %if release && regkeys && oneapi-readelf %{ oneapi-readelf --debug-dump=info %t_OCL_simd32_size_mismatch.elf | FileCheck %s --check-prefix=DWARF %}

; ASSERT: {{[Aa]ssertion.*(DW_AT_const_value size does not match the variable type size|SizeMatches)}}

; DWARF:      DW_TAG_variable
; DWARF-NEXT: DW_AT_name{{.*}}: varWide
; DWARF-NEXT: DW_AT_decl_file
; DWARF-NEXT: DW_AT_decl_line
; DWARF-NEXT: DW_AT_type
; DWARF-NOT:  DW_AT_const_value

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @size_mismatch(ptr addrspace(1) %out) !dbg !10 {
  call void @llvm.dbg.value(metadata <2 x i32> <i32 1, i32 2>, metadata !14, metadata !DIExpression()), !dbg !17
  store i64 7, ptr addrspace(1) %out, align 8, !dbg !17
  ret void, !dbg !17
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: false, emissionKind: FullDebug, enums: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!4 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "item", file: !2, line: 5, size: 192, flags: DIFlagTypePassByValue, elements: !3)
!5 = distinct !DISubroutineType(types: !3)
!10 = distinct !DISubprogram(name: "size_mismatch", scope: null, file: !2, line: 10, type: !5, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !1, retainedNodes: !3)
!14 = !DILocalVariable(name: "varWide", scope: !10, file: !2, line: 11, type: !4)
!17 = !DILocation(line: 14, column: 5, scope: !10)
