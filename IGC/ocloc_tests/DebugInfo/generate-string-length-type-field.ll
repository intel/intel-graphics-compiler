;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test checks if DW_AT_string_length is correctly generated for
; vla fortran string array (DIStringType)

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-14-plus, dg2-supported

; RUN: llvm-as -opaque-pointers=0 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd8_foo.elf | FileCheck %s

; CHECK:   DW_AT_name        : vla_string
; CHECK:   DW_AT_type        : {{.*}}

; CHECK: <{{[0-9]+}}><[[LENGHT:[0-9a-f]+]]>: Abbrev Number: [[#]] (DW_TAG_variable)
; CHECK-NEXT:   DW_AT_name        : vla_string.len

; CHECK:   DW_AT_name        : vla_string.type
; CHECK:   DW_AT_string_length: <0x[[LENGHT]]>

define spir_func void @subprgm() #0 {
entry:
  ret void, !dbg !13
}

define spir_kernel void @foo() #0 {
entry:
  call void @subprgm()
  ret void
}

attributes #0 = { noinline nounwind optnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!1, !2}

!0 = distinct !DICompileUnit(language: DW_LANG_Fortran95, file: !3, producer: "Intel(R) Fortran 24.0-1640", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, retainedTypes: !4, globals: !4, imports: !4)
!1 = !{i32 7, !"Dwarf Version", i32 4}
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !DIFile(filename: "simple-string-type.f90", directory: "/")
!4 = !{}
!5 = distinct !DISubprogram(name: "subprgm", scope: !3, file: !3, line: 256, type: !6, scopeLine: 256, unit: !0, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{null}
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "vla_string.len", scope: !5, type: !10, flags: DIFlagArtificial)
!10 = !DIBasicType(name: "integer*8", size: 64, encoding: DW_ATE_signed)
!11 = !DILocalVariable(name: "vla_string", arg: 1, scope: !5, file: !3, line: 256, type: !12)
!12 = !DIStringType(name: "vla_string.type", stringLength: !9)
!13 = !DILocation(line: 1, column: 2, scope: !5)
