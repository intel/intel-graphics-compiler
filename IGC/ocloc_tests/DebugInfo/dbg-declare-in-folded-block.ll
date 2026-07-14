;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A local variable whose debug declare is attached inside a branch-only basic
; block must still get a DWARF location, not be demoted to optimized-out.
;
; Pattern-based ISel only lists pattern-root instructions, so a block whose
; instructions are all folded away (here %folded, a bare branch) contributes
; nothing to the VISA instruction list. DwarfDebug must still visit such blocks
; to collect the debug info attached there; otherwise the variable "myvar"
; (declared in %folded, with its alloca in the entry block) loses its location.
;
; The test is mostly for LLVM 22, but we're running it on older versions
; as well to make sure the behavior is consistent across LLVM versions.

; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-17-plus

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd8_folded_block.elf | FileCheck %s

; CHECK: DW_TAG_variable
; CHECK-NEXT: DW_AT_name{{.*}}: myvar
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @folded_block() #0 {
entry:
  %var = alloca i32, align 4
  br label %folded, !dbg !4

folded:                                           ; preds = %entry
  call void @llvm.dbg.declare(metadata ptr %var, metadata !11, metadata !DIExpression()), !dbg !13
  br label %exit

exit:                                             ; preds = %folded
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

attributes #0 = { noinline optnone }

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}
!opencl.compiler.options = !{!3}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_Fortran95, file: !2, producer: "test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!2 = !DIFile(filename: "test.f90", directory: "/tmp")
!3 = !{!"-cl-take-global-address", !"-g", !"-cl-opt-disable"}
!4 = !DILocation(line: 60, column: 9, scope: !5)
!5 = distinct !DILexicalBlock(scope: !6, file: !2, line: 52, column: 776)
!6 = distinct !DILexicalBlock(scope: !7, file: !2, line: 52, column: 776)
!7 = distinct !DISubprogram(name: "folded_block", scope: null, file: !2, line: 52, type: !8, scopeLine: 52, flags: DIFlagArtificial, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagMainSubprogram, unit: !1, templateParams: !10, retainedNodes: !10)
!8 = !DISubroutineType(types: !9)
!9 = !{null}
!10 = !{}
!11 = !DILocalVariable(name: "myvar", scope: !5, file: !2, line: 50, type: !12)
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !DILocation(line: 50, column: 25, scope: !5)
