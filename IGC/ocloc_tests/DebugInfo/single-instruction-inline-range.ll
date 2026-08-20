;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; An inlined function whose body is a single instruction still has to get a PC
; range on its DW_TAG_inlined_subroutine, otherwise the debugger cannot build
; the inline frame and reports the caller's variables instead.

; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-17-plus

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd8_single_inst_inline.elf | FileCheck %s

; CHECK:      DW_TAG_inlined_subroutine
; CHECK-NEXT: DW_AT_abstract_origin
; CHECK-NEXT: DW_AT_call_file
; CHECK-NEXT: DW_AT_call_line
; CHECK-NEXT: DW_AT_low_pc

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @single_inst_inline(ptr addrspace(1) %out) #0 {
entry:
  %v = load i32, ptr addrspace(1) %out, align 4, !dbg !4
  ; the only instruction belonging to the inlined @bump
  %inc = add i32 %v, 1, !dbg !13
  store i32 %inc, ptr addrspace(1) %out, align 4, !dbg !4
  ret void, !dbg !4
}

attributes #0 = { noinline optnone }

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}
!opencl.compiler.options = !{!3}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{!"-cl-take-global-address", !"-g", !"-cl-opt-disable"}
!4 = !DILocation(line: 60, column: 9, scope: !5)
!5 = distinct !DISubprogram(name: "single_inst_inline", scope: null, file: !2, line: 52, type: !6, scopeLine: 52, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !1, templateParams: !8, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{null}
!8 = !{}
!11 = distinct !DISubprogram(name: "bump", linkageName: "bump", scope: null, file: !2, line: 30, type: !6, scopeLine: 30, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !1, templateParams: !8, retainedNodes: !8)
!13 = !DILocation(line: 31, column: 10, scope: !11, inlinedAt: !4)
