;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test checks if relocations are emitted in .debug_frame section.
; These relocations are expanded in CIE offset field of each FDE.
; There are 2 functions, so there are 2 FDEs and hence 2 relocations
; must be emitted that refer to .debug_frame.

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus, dg2-supported

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf -r %t_OCL_simd8_caller.elf | FileCheck %s

; CHECK: Relocation section '.rela.debug_frame'
; CHECK: .debug_frame + [[#]]
; CHECK: .text.caller + [[#]]
; CHECK: .debug_frame + [[#]]
; CHECK: .text.caller + [[#]]

define spir_func i32 @callee(i32 %x) #1 !dbg !10 {
entry:
  %add = add nsw i32 %x, 5, !dbg !11
  ret i32 %add, !dbg !11
}

define spir_kernel void @caller(ptr addrspace(1) %output) #0 !dbg !20 {
entry:
  %call = call spir_func i32 @callee(i32 7) #0, !dbg !21
  store i32 %call, ptr addrspace(1) %output, align 4, !dbg !22
  ret void, !dbg !23
}

attributes #0 = { convergent nounwind }
attributes #1 = { convergent noinline nounwind optnone "visaStackCall" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "test.cl", directory: "/tmp")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !DISubroutineType(types: !2)
!10 = distinct !DISubprogram(name: "callee", scope: !1, file: !1, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!11 = !DILocation(line: 2, column: 1, scope: !10)
!20 = distinct !DISubprogram(name: "caller", scope: !1, file: !1, line: 4, type: !5, scopeLine: 4, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!21 = !DILocation(line: 5, column: 3, scope: !20)
!22 = !DILocation(line: 6, column: 1, scope: !20)
!23 = !DILocation(line: 6, column: 1, scope: !20)
