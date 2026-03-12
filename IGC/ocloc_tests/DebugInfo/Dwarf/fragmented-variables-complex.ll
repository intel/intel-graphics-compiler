;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test for fragmented variable support in DWARF debug info. Motivated by Fortran
; COMPLEX variables — Flang decomposes them into real/imaginary fragments using
; DW_OP_LLVM_fragment, which IGC must emit as composite DW_OP_bit_piece
; expressions in .debug_loc.
;
; Single kernel with four COMPLEX*8 variables, each exercising a different
; cases of fragment emission:
;
;   var1_reg   — both fragments in registers
;   var2_const — both fragments are constants (DW_OP_implicit_value)
;   var3_mix   — real in register, imag is constant
;   var4_undef — real killed by undef mid-range (DW_OP_bit_piece describing empty fragment)

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_test_fragmented_complex.elf | FileCheck %s

; CHECK:      DW_AT_name        : var1_reg
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : {{.*}} (location list)

; CHECK:      DW_AT_name        : var2_const
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : {{.*}} (location list)

; CHECK:      DW_AT_name        : var3_mix
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : {{.*}} (location list)

; CHECK:      DW_AT_name        : var4_undef
; CHECK-NEXT: DW_AT_decl_file   : {{.*}}
; CHECK-NEXT: DW_AT_decl_line   : {{.*}}
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : {{.*}} (location list)

; CHECK: Contents of the .debug_loc section:
define spir_kernel void @test_fragmented_complex(ptr addrspace(1) %src, ptr addrspace(1) %out1, ptr addrspace(1) %out2, ptr addrspace(1) %out3, ptr addrspace(1) %out4) #0 !dbg !6 {
entry:
  %0 = load i64, ptr addrspace(1) %src, align 8
  %complex = bitcast i64 %0 to <2 x float>
  %real = extractelement <2 x float> %complex, i32 0
  %imag = extractelement <2 x float> %complex, i32 1

  ; var1_reg: both fragments in registers
  %real_add = fadd float %real, 1.000000e+00
  %imag_add = fadd float %imag, 2.000000e+00
  call void @llvm.dbg.value(metadata float %real_add, metadata !11, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !15
  call void @llvm.dbg.value(metadata float %imag_add, metadata !11, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !15
  store float %real_add, ptr addrspace(1) %out1, align 4, !dbg !16
  store float %imag_add, ptr addrspace(1) %out2, align 4, !dbg !16
  ; CHECK: {{.*}} ({{.*}}DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 ; {{.*}}DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 )

  ; var2_const: both values are immediates
  call void @llvm.dbg.value(metadata i32 0, metadata !12, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !12, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !17
  ; CHECK: {{.*}} (DW_OP_implicit_value {{.*}}; DW_OP_piece: 4; DW_OP_implicit_value {{.*}}; DW_OP_piece: 4)

  ; var3_mix: real in register (chained from var1_reg), imag is constant 0
  %real_mix = fadd float %real_add, 3.000000e+00
  call void @llvm.dbg.value(metadata float %real_mix, metadata !13, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !18
  call void @llvm.dbg.value(metadata i32 0,           metadata !13, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !18
  store float %real_mix, ptr addrspace(1) %out3, align 4, !dbg !19
  ; CHECK: {{.*}} ({{.*}}DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 ; DW_OP_implicit_value {{.*}}; DW_OP_piece: 4)

  ; var4_undef: real killed by undef mid-range. Uses %imag_add (from var1_reg) to keep it alive across the kernel.
  call void @llvm.dbg.value(metadata float %real_mix, metadata !14, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !20
  call void @llvm.dbg.value(metadata float %imag_add, metadata !14, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 32)), !dbg !20
  store float %real_mix, ptr addrspace(1) %out4, align 4, !dbg !21
  ; CHECK: {{.*}} ({{.*}}DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 ; {{.*}}DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 )

  ; Kill the real part — value optimized away after the store above.
  call void @llvm.dbg.value(metadata float undef, metadata !14, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32)), !dbg !22
  %imag_work = fadd float %imag_add, 1.600000e+01, !dbg !23
  ; CHECK: {{.*}} (DW_OP_piece: 4;{{.*}}DW_OP_stack_value; DW_OP_bit_piece: size: 32 offset: 0 )

  store float %imag_work, ptr addrspace(1) %out1, align 4, !dbg !24
  ret void
}

; Function Attrs: nounwind
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{i32 2, !"Dwarf Version", i32 4}
!2 = distinct !DICompileUnit(language: DW_LANG_Fortran90, file: !3, producer: "Fortran Compiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !4)
!3 = !DIFile(filename: "complex_test.f90", directory: "/tmp")
!4 = !{}
!5 = !DIBasicType(name: "COMPLEX*8", size: 64, encoding: DW_ATE_complex_float)
!6 = distinct !DISubprogram(name: "test_fragmented_complex", scope: !3, file: !3, line: 10, type: !7, isLocal: false, isDefinition: true, scopeLine: 10, isOptimized: false, unit: !2, retainedNodes: !9)
!7 = !DISubroutineType(types: !8)
!8 = !{null}
!9 = !{!11, !12, !13, !14}
!10 = distinct !DILexicalBlock(scope: !6, file: !3, line: 11, column: 1)
!11 = !DILocalVariable(name: "var1_reg",   scope: !10, file: !3, line: 12, type: !5)
!12 = !DILocalVariable(name: "var2_const", scope: !10, file: !3, line: 13, type: !5)
!13 = !DILocalVariable(name: "var3_mix",   scope: !10, file: !3, line: 14, type: !5)
!14 = !DILocalVariable(name: "var4_undef", scope: !10, file: !3, line: 15, type: !5)
!15 = !DILocation(line: 20, column: 3, scope: !10)
!16 = !DILocation(line: 21, column: 3, scope: !10)
!17 = !DILocation(line: 25, column: 3, scope: !10)
!18 = !DILocation(line: 30, column: 3, scope: !10)
!19 = !DILocation(line: 31, column: 3, scope: !10)
!20 = !DILocation(line: 35, column: 3, scope: !10)
!21 = !DILocation(line: 36, column: 3, scope: !10)
!22 = !DILocation(line: 37, column: 3, scope: !10)
!23 = !DILocation(line: 38, column: 3, scope: !10)
!24 = !DILocation(line: 39, column: 3, scope: !10)
