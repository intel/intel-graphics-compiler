;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; DW_OP_LLVM_fragment is resolved to DW_OP_bit_piece: size, offset. Test checks
; if needed DW_OP_stack_value is generated before DW_OP_bit_piece, and not at
; the end of expression in .debug_loc.

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: Contents of the .debug_loc section:
; CHECK: {{0+}} {{[0-9A-Fa-f]+}} {{[0-9A-Fa-f]+}} ({{.+}}; DW_OP_stack_value; DW_OP_bit_piece: size: [[#]] offset: [[#]] )

define spir_kernel void @foo(ptr addrspace(1) %src, ptr addrspace(1) %out_real, ptr addrspace(1) %out_imag) #0 !dbg !5 {
entry:
  %0 = load i64, ptr addrspace(1) %src, align 8
  %complex = bitcast i64 %0 to <2 x float>
  %real_part = extractelement <2 x float> %complex, i32 0
  %imag_part = extractelement <2 x float> %complex, i32 1

  ; Assumes real part is optimized out
  call void @llvm.dbg.value(metadata float undef, metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !12
  call void @llvm.dbg.value(metadata float %imag_part, metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !12
  store float %real_part, ptr addrspace(1) %out_real, align 4, !dbg !13
  store float %imag_part, ptr addrspace(1) %out_imag, align 4, !dbg !13
  ret void
}

; Function Attrs: nounwind
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #1 = { nounwind readnone }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{i32 2, !"Dwarf Version", i32 4}
!2 = distinct !DICompileUnit(language: DW_LANG_Fortran90, file: !3, producer: "Fortran Compiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !4)
!3 = !DIFile(filename: "complex_test.f90", directory: "/tmp")
!4 = !{}
!5 = distinct !DISubprogram(name: "foo", scope: !3, file: !3, line: 1, type: !6, isLocal: false, isDefinition: true, scopeLine: 1, isOptimized: false, unit: !2, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{null}
!8 = !{!9}
!9 = !DILocalVariable(name: "complex", scope: !11, file: !3, line: 3, type: !10)
!10 = !DIBasicType(name: "COMPLEX*8", size: 64, encoding: DW_ATE_complex_float)
!11 = distinct !DILexicalBlock(scope: !5, file: !3, line: 2, column: 1)
!12 = !DILocation(line: 3, column: 3, scope: !11)
!13 = !DILocation(line: 4, column: 4, scope: !11)
