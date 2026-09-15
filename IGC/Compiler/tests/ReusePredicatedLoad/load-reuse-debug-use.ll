;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-20-plus
; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; Debug info neither blocks reuse nor is invalidated by it.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; Two loads that differ only in their !dbg location are still the same memory
; operation, so the second is reused. The surviving load keeps its own location.
; CHECK-LABEL: define i32 @different_debug_loc(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !dbg ![[LOCA:[0-9]+]]
; CHECK-NEXT:    %r = add i32 %a, %a
; CHECK-NEXT:    ret i32 %r

define i32 @different_debug_loc(ptr addrspace(1) %p, i1 noundef %q) !dbg !4 {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !dbg !9
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !dbg !10
  %r = add i32 %a, %b
  ret i32 %r
}

; A debug record is not in the use list, so it does not make %b look used outside
; its predicate. Load is reused and the debug record is retargeted at the reused value
; rather than left dangling.
; CHECK-LABEL: define float @debug_record_use(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %mask = and i1 %S, %A
; CHECK-NEXT:    #dbg_value(i32 %a, {{.*}})
; CHECK-NEXT:    %af = bitcast i32 %a to float
; CHECK-NEXT:    %bf = bitcast i32 %a to float
; CHECK-NEXT:    %result = select i1 %mask, float %bf, float %af
; CHECK-NEXT:    ret float %result

define float @debug_record_use(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) !dbg !5 {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0), !dbg !12
  %mask = and i1 %S, %A
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 37), !dbg !12
    #dbg_value(i32 %b, !8, !DIExpression(), !12)
  %af = bitcast i32 %a to float
  %bf = bitcast i32 %b to float
  %result = select i1 %mask, float %bf, float %af
  ret float %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn memory(read) }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!3 = !DIFile(filename: "load-reuse-debug-use.ll", directory: "/")
!4 = distinct !DISubprogram(name: "different_debug_loc", scope: !3, file: !3, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !7)
!5 = distinct !DISubprogram(name: "debug_record_use", scope: !3, file: !3, line: 3, type: !6, scopeLine: 3, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !7)
!6 = !DISubroutineType(types: !7)
!7 = !{}
!8 = !DILocalVariable(name: "b", scope: !5, file: !3, line: 3, type: !11)
!9 = !DILocation(line: 1, column: 1, scope: !4)
!10 = !DILocation(line: 2, column: 1, scope: !4)
!11 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!12 = !DILocation(line: 3, column: 1, scope: !5)
