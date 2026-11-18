;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A constant dbg.value operand is the input of its DIExpression: .debug_loc must
; push the constant (DW_OP_consts for a signed variable, DW_OP_constu for an unsigned
; one) and then apply the expression, instead of emitting dbg.value.

; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-16-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=loc %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: (DW_OP_consts: 5; DW_OP_plus_uconst: 1; DW_OP_stack_value)
; CHECK: (DW_OP_consts: 5; DW_OP_plus_uconst: 2; DW_OP_stack_value)
; CHECK: (DW_OP_consts: -1; DW_OP_plus_uconst: 2; DW_OP_stack_value)
; CHECK: (DW_OP_constu: 32; DW_OP_plus_uconst: 1; DW_OP_stack_value)
; CHECK: (DW_OP_constu: 0; DW_OP_plus_uconst: 1; DW_OP_stack_value)

define spir_kernel void @foo(ptr addrspace(1) %out) !dbg !4 {
  call void @llvm.dbg.value(metadata i32 5, metadata !7, metadata !DIExpression(DW_OP_plus_uconst, 1, DW_OP_stack_value)), !dbg !10
  call void @llvm.dbg.value(metadata i32 32, metadata !8, metadata !DIExpression(DW_OP_plus_uconst, 1, DW_OP_stack_value)), !dbg !10
  store volatile i32 0, ptr addrspace(1) %out, align 4, !dbg !10
  call void @llvm.dbg.value(metadata i32 5, metadata !7, metadata !DIExpression(DW_OP_plus_uconst, 2, DW_OP_stack_value)), !dbg !10
  call void @llvm.dbg.value(metadata i32 0, metadata !8, metadata !DIExpression(DW_OP_plus_uconst, 1, DW_OP_stack_value)), !dbg !10
  store volatile i32 1, ptr addrspace(1) %out, align 4, !dbg !10
  call void @llvm.dbg.value(metadata i32 -1, metadata !7, metadata !DIExpression(DW_OP_plus_uconst, 2, DW_OP_stack_value)), !dbg !10
  store volatile i32 2, ptr addrspace(1) %out, align 4, !dbg !10
  ret void, !dbg !10
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_OpenCL, file: !2, emissionKind: FullDebug)
!2 = !DIFile(filename: "a.cl", directory: "/")
!3 = !DISubroutineType(types: !{})
!4 = distinct !DISubprogram(name: "foo", file: !2, type: !3, spFlags: DISPFlagDefinition, unit: !1)
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !DIBasicType(name: "uint", size: 32, encoding: DW_ATE_unsigned)
!7 = !DILocalVariable(name: "s", scope: !4, type: !5)
!8 = !DILocalVariable(name: "u", scope: !4, type: !6)
!10 = !DILocation(line: 1, scope: !4)
