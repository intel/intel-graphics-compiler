;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verify new DWARF v5 sections: .debug_loclists, .debug_rnglists, .debug_addr and
; relocation reduction.

; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-14-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf -w -r -W %t_OCL_simd32_foo.elf &> %t_readelf.dwarf
; RUN: FileCheck %s --input-file=%t_readelf.dwarf

; ===== relocations ===== ;
; .debug_info references loclists, rnglists and addr via one relocation each
; CHECK:      Relocation section '.rela.debug_info'
; CHECK:      .debug_addr
; CHECK:      .debug_loclists
; CHECK:      .debug_rnglists
;
; .debug_addr has relocations to .text (addresses in the pool)
; CHECK:      Relocation section '.rela.debug_addr'
; CHECK:      .text.foo
; CHECK:      .text.foo
;
; No relocation sections for .debug_loclists or .debug_rnglists
; CHECK-NOT:  .rela.debug_loclists
; CHECK-NOT:  .rela.debug_rnglists
;
; ===== .debug_info ===== ;
; CHECK:      Version:       5
; CHECK:      Unit Type:     DW_UT_compile (1)
;
; CU addr base attribute
; CHECK:      DW_AT_addr_base   : (sec_offset)
;
; CU low_pc via .debug_addr index, high_pc as size delta
; CHECK:      DW_AT_low_pc      : (addrx)
; CHECK:      DW_AT_high_pc     : (data4)
;
; CU loclists/rnglists base attributes
; CHECK:      DW_AT_loclists_base: (sec_offset)
; CHECK:      DW_AT_rnglists_base: (sec_offset)

; subprogram
; CHECK:      DW_TAG_subprogram
; CHECK:      DW_AT_low_pc      : (addrx)
; CHECK:      DW_AT_high_pc     : (data4)

; variables use loclistx
; CHECK:      DW_AT_name        : (string) pint
; CHECK:      DW_AT_location    : (loclistx)
; CHECK:      DW_AT_name        : (string) vint
; CHECK:      DW_AT_location    : (loclistx)

; lexical block uses rnglistx
; CHECK:      DW_TAG_lexical_block
; CHECK:      DW_AT_ranges      : (rnglistx)

; variables use loclistx
; CHECK:      DW_AT_name        : (string) t
; CHECK:      DW_AT_location    : (loclistx)

; ===== .debug_abbrev ===== ;
; CHECK:      DW_AT_addr_base    DW_FORM_sec_offset
; CHECK:      DW_AT_low_pc       DW_FORM_addrx
; CHECK:      DW_AT_high_pc      DW_FORM_data4
; CHECK:      DW_AT_loclists_base DW_FORM_sec_offset
; CHECK:      DW_AT_rnglists_base DW_FORM_sec_offset
; CHECK:      DW_AT_location     DW_FORM_loclistx
; CHECK:      DW_AT_ranges       DW_FORM_rnglistx

; ===== .debug_addr ===== ;
; CHECK:      Contents of the .debug_addr section:
; CHECK:      Index  Address
; CHECK-NEXT: 0:
; CHECK-NEXT: 1:

; ===== .debug_loclists ===== ;
; CHECK:      Contents of the .debug_loclists section:
; CHECK:      DWARF version:   5
; CHECK:      Offset entries:  3
;
; CHECK:      Offset Entry 0
; CHECK:      <End of list>
;
; CHECK:      Offset Entry 1
; CHECK:      <End of list>
;
; CHECK:      Offset Entry 2
; CHECK:      <End of list>

; ===== .debug_rnglists ===== ;
; CHECK:      Contents of the .debug_rnglists section:
; CHECK:      DWARF version:   5
; CHECK:      Offset entries:  1
; CHECK:      <End of list>

define spir_kernel void @foo(i32 addrspace(1)* %p, i32 addrspace(1)* %q) #0 !dbg !7 {
entry:
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %p, metadata !11, metadata !DIExpression(DW_OP_deref)), !dbg !16
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %q, metadata !12, metadata !DIExpression(DW_OP_deref)), !dbg !16
  br label %blk1, !dbg !16

blk1:
  store volatile i32 1, i32 addrspace(1)* %p, align 4, !dbg !18
  call void @llvm.dbg.value(metadata i32 10, metadata !13, metadata !DIExpression()), !dbg !18
  br label %mid, !dbg !18

mid:
  store volatile i32 2, i32 addrspace(1)* %q, align 4, !dbg !17
  br label %blk2, !dbg !17

blk2:
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %p, metadata !11, metadata !DIExpression(DW_OP_deref)), !dbg !18
  call void @llvm.dbg.value(metadata i32 20, metadata !13, metadata !DIExpression()), !dbg !18
  store volatile i32 3, i32 addrspace(1)* %p, align 4, !dbg !18
  ret void, !dbg !18
}

declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { nounwind }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!opencl.spir.version = !{!4}
!opencl.ocl.version = !{!4}

!0 = !{i32 7, !"Dwarf Version", i32 5}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!3 = !DIFile(filename: "test.cl", directory: ".")
!4 = !{i32 2, i32 0}
!5 = !{null}
!6 = !DISubroutineType(types: !5)
!7 = distinct !DISubprogram(name: "foo", file: !3, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !8)
!8 = !{!11, !12}
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!11 = !DILocalVariable(name: "pint", scope: !7, file: !3, line: 1, type: !10)
!12 = !DILocalVariable(name: "vint", scope: !7, file: !3, line: 1, type: !9)
!13 = !DILocalVariable(name: "t", scope: !14, file: !3, line: 2, type: !9)
!14 = distinct !DILexicalBlock(scope: !7, file: !3, line: 2)
!15 = distinct !DILexicalBlock(scope: !7, file: !3, line: 3)
!16 = !DILocation(line: 1, scope: !7)
!17 = !DILocation(line: 3, scope: !15)
!18 = !DILocation(line: 2, scope: !14)
