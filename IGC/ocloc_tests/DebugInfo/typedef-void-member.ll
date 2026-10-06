;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; CodeView-style debug info lists a nested typedef in its struct's elements. Here the typedef
; names void (baseType: null), as in conditional<false, T, void>::type. The struct DIE must keep
; the typedef as a child without a DW_AT_type.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus, regkeys, oneapi-readelf

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd32_typedef_void_member.elf | FileCheck %s

; CHECK:      DW_TAG_structure_type
; CHECK-NEXT: DW_AT_name{{.*}}: conditional<0,float,void>
; CHECK:      DW_TAG_typedef
; CHECK-NEXT: DW_AT_name{{.*}}: type
; CHECK-NOT:  DW_AT_type
; CHECK:      Abbrev Number: 0

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @typedef_void_member(ptr addrspace(1) %out) !dbg !10 {
  %c = alloca i8, align 1
  call void @llvm.dbg.declare(metadata ptr %c, metadata !14, metadata !DIExpression()), !dbg !17
  store i8 0, ptr %c, align 1, !dbg !17
  %v = load i8, ptr %c, align 1, !dbg !17
  store i8 %v, ptr addrspace(1) %out, align 1, !dbg !17
  ret void, !dbg !17
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: false, emissionKind: FullDebug, enums: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!5 = distinct !DISubroutineType(types: !3)
!6 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "conditional<0,float,void>", file: !2, line: 3, size: 8, flags: DIFlagTypePassByValue, elements: !7, identifier: "_ZTS11conditionalILb0EfvE")
!7 = !{!8}
!8 = !DIDerivedType(tag: DW_TAG_typedef, name: "type", scope: !6, file: !2, line: 3, baseType: null)
!10 = distinct !DISubprogram(name: "typedef_void_member", scope: null, file: !2, line: 10, type: !5, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !1, retainedNodes: !3)
!14 = !DILocalVariable(name: "c", scope: !10, file: !2, line: 11, type: !6)
!17 = !DILocation(line: 12, column: 5, scope: !10)
