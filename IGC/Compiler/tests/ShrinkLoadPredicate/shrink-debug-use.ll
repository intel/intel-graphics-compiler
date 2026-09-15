;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-20-plus
; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; A debug record is not an observable use, so it neither blocks shrinking nor
; contributes a condition.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @debug_use(
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond, !dbg !{{[0-9]+}}
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0), !dbg !{{[0-9]+}}
; CHECK-NEXT:    #dbg_value(i32 %a, {{.*}})
; CHECK-NEXT:    %r = select i1 %cond, i32 %a, i32 %other, !dbg !{{[0-9]+}}
; CHECK-NEXT:    ret i32 %r, !dbg !{{[0-9]+}}

define i32 @debug_use(ptr addrspace(1) %p, i1 %q, i1 noundef %cond, i32 %other) !dbg !4 {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !dbg !9
    #dbg_value(i32 %a, !8, !DIExpression(), !9)
  %r = select i1 %cond, i32 %a, i32 %other, !dbg !9
  ret i32 %r, !dbg !9
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn memory(read) }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!3 = !DIFile(filename: "shrink-debug-use.ll", directory: "/")
!4 = distinct !DISubprogram(name: "debug_use", scope: !3, file: !3, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !7)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = !{}
!8 = !DILocalVariable(name: "a", scope: !4, file: !3, line: 1, type: !10)
!9 = !DILocation(line: 1, column: 1, scope: !4)
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
