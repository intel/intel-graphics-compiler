;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Checks if DW_AT_trampoline is correctly emitted from DISubprogram::targetFuncName.

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-15-plus, dg2-supported

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd8_foo.elf | FileCheck %s

; CHECK:      DW_AT_name        : fourth_.t75p.t76p
; CHECK:      DW_AT_trampoline  : fourth_{{$}}

define spir_func void @"fourth_.t75p.t76p"() #0 !dbg !5 {
  ret void, !dbg !7
}

define spir_kernel void @foo() !dbg !6 {
  call spir_func void @"fourth_.t75p.t76p"(), !dbg !8
  ret void
}

attributes #0 = { noinline nounwind optnone "visaStackCall" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_Fortran95, file: !1, producer: "ifx", emissionKind: FullDebug)
!1 = !DIFile(filename: "call-stack.f90", directory: "trampoline")
!2 = !{i32 2, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !DISubroutineType(types: !{})
!5 = distinct !DISubprogram(name: "fourth_.t75p.t76p", file: !1, type: !4, spFlags: DISPFlagDefinition, unit: !0, targetFuncName: "fourth_")
!6 = distinct !DISubprogram(name: "foo", file: !1, type: !4, spFlags: DISPFlagDefinition, unit: !0)
!7 = !DILocation(line: 1, scope: !5)
!8 = !DILocation(line: 2, scope: !6)
