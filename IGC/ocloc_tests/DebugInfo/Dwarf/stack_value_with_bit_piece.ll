;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; DW_OP_LLVM_fragment is resolved to DW_OP_bit_piece: size, offset.
; Test checks three scenarios:
; - DW_OP_stack_value is generated when absent from the source DIExpression.
; - DW_OP_stack_value is not duplicated when already present.
; - DW_OP_stack_value is correctly emitted for a sliced SIMD32 fragment.

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: Contents of the .debug_loc section:

; complex
; CHECK: {{.*}} DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0
; CHECK-NEXT: <End of list>

; complex_implicit
; CHECK-NEXT: {{.*}} DW_OP_and; DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 ; {{.*}} DW_OP_shr; {{.*}} DW_OP_and; DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0
; CHECK-NEXT: <End of list>

; complex_sliced
; CHECK-NEXT: (DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_ge; DW_OP_bra: 20; {{.*}} DW_OP_INTEL_regval_bits: 32; DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 ; DW_OP_skip: 19; {{.*}} DW_OP_INTEL_regval_bits: 32; DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 ; DW_OP_implicit_value 4 byte block: 0 0 80 3f ; DW_OP_piece: 4)

define spir_kernel void @foo(ptr addrspace(1) %src, ptr addrspace(1) %out_real, ptr addrspace(1) %out_imag, ptr addrspace(1) %out_sliced) #0 !dbg !5 {
entry:
  %0 = load i64, ptr addrspace(1) %src, align 8
  %complex = bitcast i64 %0 to <2 x float>
  %real_part = extractelement <2 x float> %complex, i32 0
  %imag_part = extractelement <2 x float> %complex, i32 1

  ; No source DW_OP_stack_value.
  call void @llvm.dbg.value(metadata float undef, metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !14
  call void @llvm.dbg.value(metadata float %imag_part, metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !14

  ; Source DW_OP_stack_value.
  call void @llvm.dbg.value(metadata float %real_part, metadata !10, metadata !DIExpression(DW_OP_LLVM_convert, 64, DW_ATE_unsigned, DW_OP_LLVM_convert, 32, DW_ATE_unsigned, DW_OP_stack_value, DW_OP_LLVM_fragment, 0, 32)), !dbg !14
  call void @llvm.dbg.value(metadata float %imag_part, metadata !10, metadata !DIExpression(DW_OP_constu, 32, DW_OP_shr, DW_OP_LLVM_convert, 64, DW_ATE_unsigned, DW_OP_LLVM_convert, 32, DW_ATE_unsigned, DW_OP_stack_value, DW_OP_LLVM_fragment, 32, 32)), !dbg !14

  ; Sliced SIMD32 fragment and immediate fragment.
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %gid32 = trunc i64 %gid to i32
  %sliced = uitofp i32 %gid32 to float
  call void @llvm.dbg.value(metadata float %sliced, metadata !11, metadata !DIExpression(DW_OP_stack_value, DW_OP_LLVM_fragment, 0, 32)), !dbg !14
  call void @llvm.dbg.value(metadata float 1.000000e+00, metadata !11, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !14
  store float %real_part, ptr addrspace(1) %out_real, align 4, !dbg !15
  store float %imag_part, ptr addrspace(1) %out_imag, align 4, !dbg !15
  store float %sliced, ptr addrspace(1) %out_sliced, align 4, !dbg !15
  ret void
}

; Function Attrs: nounwind
declare void @llvm.dbg.value(metadata, metadata, metadata) #1
declare spir_func i64 @_Z13get_global_idj(i32)

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
!8 = !{!9, !10, !11}
!9 = !DILocalVariable(name: "complex", scope: !13, file: !3, line: 3, type: !12)
!10 = !DILocalVariable(name: "complex_implicit", scope: !13, file: !3, line: 3, type: !12)
!11 = !DILocalVariable(name: "complex_sliced", scope: !13, file: !3, line: 3, type: !12)
!12 = !DIBasicType(name: "COMPLEX*8", size: 64, encoding: DW_ATE_complex_float)
!13 = distinct !DILexicalBlock(scope: !5, file: !3, line: 2, column: 1)
!14 = !DILocation(line: 3, column: 3, scope: !13)
!15 = !DILocation(line: 4, column: 4, scope: !13)
