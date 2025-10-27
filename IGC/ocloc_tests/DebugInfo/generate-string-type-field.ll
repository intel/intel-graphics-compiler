;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test whether DW_TAG_string_type is generated in DWARF

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, dg2-supported

; RUN: llvm-as -opaque-pointers=0 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: DW_TAG_string_type
; CHECK:   DW_AT_name        : character(*)
; CHECK: DW_TAG_string_type
; CHECK:   DW_AT_name        : character(10)
; CHECK:   DW_AT_byte_size   : 10
; CHECK: DW_TAG_string_type
; CHECK:   DW_AT_name        : character
; CHECK:   DW_AT_encoding    : 0    (void)
; CHECK:   DW_AT_byte_size   : 0

; Function Attrs: noinline nounwind optnone
define spir_kernel void @foo() #0 {
entry:
  ret void
}

attributes #0 = { noinline nounwind optnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!1, !2}

!0 = distinct !DICompileUnit(language: DW_LANG_Fortran95, file: !3, producer: "Intel(R) Fortran 24.0-1640", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, retainedTypes: !5, globals: !4, imports: !4)
!1 = !{i32 7, !"Dwarf Version", i32 4}
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !DIFile(filename: "simple-string-type.f90", directory: "/")
!4 = !{}
!5 = !{!6, !9, !12, !13}
!6 = !DIStringType(name: "character(*)", stringLength: !7, stringLengthExpression: !DIExpression(), size: 32)
!7 = !DILocalVariable(arg: 2, scope: !8, file: !3, line: 256, type: !11, flags: DIFlagArtificial)
!8 = distinct !DISubprogram(name: "subprgm", scope: !0, file: !3, line: 256, type: !9, isLocal: false, isDefinition: true, scopeLine: 256, isOptimized: false, unit: !0)
!9 = !DISubroutineType(types: !10)
!10 = !{null, !6, !11}
!11 = !DIBasicType(name: "integer*8", size: 64, align: 64, encoding: DW_ATE_signed)
!12 = !DIStringType(name: "character(10)", size: 80, align: 8)
!13 = !DIBasicType(tag: DW_TAG_string_type, name: "character")
