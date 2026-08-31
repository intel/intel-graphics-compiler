;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A dbg.value sits in a self-looping block with no debug-annotated instruction
; after it, so the variable's [start, end] range inverts (end is emitted before
; start). No location can be emitted for such a variable; this test only checks
; that IGC does not crash on the reversed range.

; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-17-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd32_dbg_value_selfloop.elf | FileCheck %s

; CHECK: DW_TAG_variable
; CHECK-NEXT: DW_AT_name{{.*}}: myvar
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type

; Don't emit inverted range
; CHECK-NOT: DW_AT_location

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @dbg_value_selfloop(ptr addrspace(1) %0, i1 %1) {
  %3 = load i64, ptr null, align 8, !dbg !4 ; last actual instruction with !dbg, End will be assigned here
  br i1 %1, label %4, label %.loopexit

.loopexit:                                        ; preds = %.loopexit, %2
  call void @llvm.dbg.value(metadata ptr addrspace(1) %0, metadata !10, metadata !DIExpression()), !dbg !13 ; Start on debug intrinsics
  br label %.loopexit ; Start on debug records

4:                                                ; preds = %2
  ret void
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: false, emissionKind: FullDebug, imports: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!4 = !DILocation(line: 255, column: 1, scope: !5)
!5 = distinct !DILexicalBlock(scope: !6, file: !2, line: 255, column: 209)
!6 = distinct !DILexicalBlock(scope: !7, file: !2, line: 247, column: 209)
!7 = distinct !DISubprogram(name: "dbg_value_selfloop", scope: null, file: !2, line: 247, type: !8, scopeLine: 247, flags: DIFlagArtificial, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !1, templateParams: !3, retainedNodes: !3)
!8 = !DISubroutineType(types: !9)
!9 = !{null}
!10 = !DILocalVariable(name: "myvar", scope: !7, file: !2, line: 242, type: !11)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64, dwarfAddressSpace: 4)
!12 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!13 = !DILocation(line: 0, scope: !7)
