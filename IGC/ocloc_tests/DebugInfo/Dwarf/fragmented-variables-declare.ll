;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test for fragmented variable support in DWARF debug info using dbg.declare
; DW_OP_LLVM_fragment combined with struct types.
;
;   struct Pair {
;       int   x;    // 32 bits, fragment(0, 32)
;       int   y;    // 32 bits, fragment(32, 32)
;   };
;
; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_test_fragmented_declare.elf | FileCheck %s

; CHECK:      DW_AT_name        : pair_struct
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : {{.*}} (location list)

%struct.Pair = type { i32, i32 }

; CHECK: Contents of the .debug_loc section:
define spir_kernel void @test_fragmented_declare(ptr addrspace(1) %dst, ptr addrspace(1) %out) #0 !dbg !10 {
  %ptr_x = getelementptr inbounds %struct.Pair, ptr addrspace(1) %dst, i64 0, i32 0, !dbg !16
  call void @llvm.dbg.declare(metadata ptr addrspace(1) %ptr_x, metadata !15, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !16
  store i32 14, ptr addrspace(1) %ptr_x, align 4, !dbg !16

  %ptr_y = getelementptr inbounds %struct.Pair, ptr addrspace(1) %dst, i64 0, i32 1, !dbg !17
  call void @llvm.dbg.declare(metadata ptr addrspace(1) %ptr_y, metadata !15, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !17
  store i32 26, ptr addrspace(1) %ptr_y, align 4, !dbg !17

  ; Each fragment is described as separate pointer
  ; CHECK: {{.*}} ({{.*}}DW_OP_INTEL_regval_bits: 64; {{.*}}DW_OP_bit_piece: size: 32 offset: 0 ; {{.*}}DW_OP_INTEL_regval_bits: 64; {{.*}}DW_OP_bit_piece: size: 32 offset: 0 )
  %1 = load i32, ptr addrspace(1) %ptr_x, align 4, !dbg !18
  %2 = load i32, ptr addrspace(1) %ptr_y, align 4, !dbg !18
  %sum = add i32 %1, %2, !dbg !19
  store i32 %sum, ptr addrspace(1) %out, align 4, !dbg !20
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{i32 2, !"Dwarf Version", i32 4}
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !4)
!3 = !DIFile(filename: "declare_struct.c", directory: "/tmp")
!4 = !{}
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !DICompositeType(tag: DW_TAG_structure_type, name: "Pair", file: !3, line: 3, size: 64, align: 32, elements: !7)
!7 = !{!8, !9}
!8 = !DIDerivedType(tag: DW_TAG_member, name: "x", scope: !6, file: !3, line: 4, baseType: !5, size: 32, align: 32)
!9 = !DIDerivedType(tag: DW_TAG_member, name: "y", scope: !6, file: !3, line: 5, baseType: !5, size: 32, align: 32, offset: 32)
!10 = distinct !DISubprogram(name: "test_fragmented_declare", scope: !3, file: !3, line: 10, type: !11, isLocal: false, isDefinition: true, scopeLine: 10, isOptimized: false, unit: !2, retainedNodes: !13)
!11 = !DISubroutineType(types: !12)
!12 = !{null}
!13 = !{!15}
!14 = distinct !DILexicalBlock(scope: !10, file: !3, line: 11, column: 1)
!15 = !DILocalVariable(name: "pair_struct", scope: !14, file: !3, line: 12, type: !6)
!16 = !DILocation(line: 12, column: 3, scope: !14)
!17 = !DILocation(line: 15, column: 3, scope: !14)
!18 = !DILocation(line: 16, column: 3, scope: !14)
!19 = !DILocation(line: 17, column: 3, scope: !14)
!20 = !DILocation(line: 18, column: 3, scope: !14)
