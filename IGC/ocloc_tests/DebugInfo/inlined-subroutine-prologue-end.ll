;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; prologue_end is set once per function. Inlined code has no prologue, so it
; counts as part of the function it is inlined into.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-17-plus

; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device dg2 -options "-g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: llvm-dwarfdump --debug-line %t_OCL_simd32_foo.elf | FileCheck %s

; Check if foo and the inlined bar have one prologue_end in total.
; CHECK:     {{^0x.*}} prologue_end
; CHECK-NOT: {{^0x.*}} prologue_end

define void @bar(ptr addrspace(1) %p) alwaysinline !dbg !3 {
  store i32 0, ptr addrspace(1) %p, !dbg !4
  ret void
}

; ret has a location, so foo has a row of its own after the inlined bar.
define spir_kernel void @foo(ptr addrspace(1) %p) !dbg !5 {
  call void @bar(ptr addrspace(1) %p), !dbg !6
  ret void, !dbg !6
}

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_OpenCL, file: !2, emissionKind: FullDebug)
!2 = !DIFile(filename: "test.cl", directory: "/")
!3 = distinct !DISubprogram(name: "bar", file: !2, type: !7, spFlags: DISPFlagDefinition, unit: !1)
!4 = !DILocation(line: 1, scope: !3)
!5 = distinct !DISubprogram(name: "foo", file: !2, type: !7, spFlags: DISPFlagDefinition, unit: !1)
!6 = !DILocation(line: 2, scope: !5)
!7 = !DISubroutineType(types: !8)
!8 = !{null}
