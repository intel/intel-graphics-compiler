;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verifies that scalar integer (ConstantInt) and float (ConstantFP) immediates are
; emitted as a DW_OP_implicit_value sub-range in .debug_loc.

; UNSUPPORTED: sys32
; UNSUPPORTED: llvm-22-plus
; FIXME: update this test for LLVM 22
; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info,loc %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: DW_AT_name {{.*}}: int_imm
; CHECK: DW_AT_location {{.*}}: {{.*}} (location list)

; CHECK: DW_AT_name {{.*}}: float_imm
; CHECK: DW_AT_location {{.*}}: {{.*}} (location list)

; int_imm = 7 -> 7 0 0 0 0 0 0 0, float_imm = 2.5 = 0x40200000 -> 0 0 20 40 0 0 0 0.
; CHECK: Contents of the .debug_loc section:
; CHECK: (DW_OP_implicit_value 8 byte block: 7 0 0 0 0 0 0 0 )
; CHECK: (DW_OP_implicit_value 8 byte block: 0 0 20 40 0 0 0 0 )

define spir_kernel void @foo(ptr addrspace(1) %out) #0 !dbg !4 {
entry:
  call void @llvm.dbg.value(metadata i32 7, metadata !9, metadata !DIExpression()), !dbg !11
  call void @llvm.dbg.value(metadata float 2.500000e+00, metadata !10, metadata !DIExpression()), !dbg !12
  %iv = load i32, ptr addrspace(1) %out, align 4
  %ir = add i32 %iv, 1
  call void @llvm.dbg.value(metadata i32 %ir, metadata !9, metadata !DIExpression()), !dbg !13
  %fv = load float, ptr addrspace(1) %out, align 4
  %fr = fadd float %fv, 1.000000e+00
  call void @llvm.dbg.value(metadata float %fr, metadata !10, metadata !DIExpression()), !dbg !14
  store i32 %ir, ptr addrspace(1) %out, align 4, !dbg !13
  store float %fr, ptr addrspace(1) %out, align 4, !dbg !14
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
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!3 = !DIFile(filename: "scalar-immediate.c", directory: "ocloc_tests")
!4 = distinct !DISubprogram(name: "foo", scope: !3, file: !3, line: 10, type: !5, isLocal: false, isDefinition: true, scopeLine: 10, isOptimized: false, unit: !2, retainedNodes: !7)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = !{!9, !10}
!8 = distinct !DILexicalBlock(scope: !4, file: !3, line: 11, column: 1)
!9 = !DILocalVariable(name: "int_imm", scope: !8, file: !3, line: 12, type: !16)
!10 = !DILocalVariable(name: "float_imm", scope: !8, file: !3, line: 13, type: !15)
!11 = !DILocation(line: 20, column: 3, scope: !8)
!12 = !DILocation(line: 21, column: 3, scope: !8)
!13 = !DILocation(line: 22, column: 3, scope: !8)
!14 = !DILocation(line: 23, column: 3, scope: !8)
!15 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!16 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
