;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test for fragmented variable support in DWARF debug info with struct types.
; A struct with fields of varying sizes is decomposed into fragments described
; by dbg.value + DW_OP_LLVM_fragment.
;
;   struct mixed {
;       char  a;    //  8 bits, fragment(0, 8)
;       short b;    // 16 bits, fragment(16, 16)
;       int   c;    // 32 bits, fragment(32, 32)
;   };
;
; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; RUN: llvm-as -opaque-pointers=1 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_test_fragmented_struct.elf | FileCheck %s

; CHECK:      DW_AT_name        : mixed
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : {{.*}} (location list)
%mixed = type { i8, i16, i32 }

; CHECK: Contents of the .debug_loc section:
define spir_kernel void @test_fragmented_struct(ptr addrspace(1) %src, ptr addrspace(1) %out1, ptr addrspace(1) %out2, ptr addrspace(1) %out3) #0 !dbg !13 {
entry:
  %ptr_a = getelementptr %mixed, ptr addrspace(1) %src, i32 0, i32 0
  %a_raw = load i8,  ptr addrspace(1) %ptr_a, align 1
  %ptr_b = getelementptr %mixed, ptr addrspace(1) %src, i32 0, i32 1
  %b_raw = load i16, ptr addrspace(1) %ptr_b, align 2
  %ptr_c = getelementptr %mixed, ptr addrspace(1) %src, i32 0, i32 2
  %c_raw = load i32, ptr addrspace(1) %ptr_c, align 4
  %a = add i8  %a_raw, 1
  %b = add i16 %b_raw, 2
  %c = add i32 %c_raw, 3
  call void @llvm.dbg.value(metadata i8  %a, metadata !18, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 8)),   !dbg !19
  call void @llvm.dbg.value(metadata i16 %b, metadata !18, metadata !DIExpression(DW_OP_LLVM_fragment, 16, 16)), !dbg !19
  call void @llvm.dbg.value(metadata i32 %c, metadata !18, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !19
  %a32   = zext i8 %a to i32
  %b32   = zext i16 %b to i32
  store i32 %a32,  ptr addrspace(1) %out1, align 4, !dbg !20
  store i32 %b32,  ptr addrspace(1) %out2, align 4, !dbg !20
  store i32 %c, ptr addrspace(1) %out3, align 4, !dbg !20
  ; CHECK: {{.*}} ({{.*}}DW_OP_bit_piece: size: 8 offset: 0 ; {{.*}}DW_OP_bit_piece: size: 16 offset: 0 ; {{.*}}DW_OP_bit_piece: size: 32 offset: 0 )
  ret void
}

declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{i32 2, !"Dwarf Version", i32 4}
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !4)
!3 = !DIFile(filename: "struct_test.c", directory: "/tmp")
!4 = !{}
!5 = !DIBasicType(name: "char",  size: 8,  encoding: DW_ATE_signed_char)
!6 = !DIBasicType(name: "short", size: 16, encoding: DW_ATE_signed)
!7 = !DIBasicType(name: "int",   size: 32, encoding: DW_ATE_signed)
!8 = !DICompositeType(tag: DW_TAG_structure_type, name: "mixed", file: !3, line: 3, size: 64, elements: !9)
!9 = !{!10, !11, !12}
!10 = !DIDerivedType(tag: DW_TAG_member, name: "a", scope: !8, file: !3, line: 4, baseType: !5, size: 8, offset: 0)
!11 = !DIDerivedType(tag: DW_TAG_member, name: "b", scope: !8, file: !3, line: 5, baseType: !6, size: 16, offset: 16)
!12 = !DIDerivedType(tag: DW_TAG_member, name: "c", scope: !8, file: !3, line: 6, baseType: !7, size: 32, offset: 32)
!13 = distinct !DISubprogram(name: "test_fragmented_struct", scope: !3, file: !3, line: 10, type: !14, isLocal: false, isDefinition: true, scopeLine: 10, isOptimized: false, unit: !2, retainedNodes: !16)
!14 = !DISubroutineType(types: !15)
!15 = !{null}
!16 = !{!18}
!17 = distinct !DILexicalBlock(scope: !13, file: !3, line: 11, column: 1)
!18 = !DILocalVariable(name: "mixed", scope: !17, file: !3, line: 12, type: !8)
!19 = !DILocation(line: 12, column: 3, scope: !17)
!20 = !DILocation(line: 13, column: 3, scope: !17)
