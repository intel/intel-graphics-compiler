;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-20-plus
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -dce -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %mid is rewritten in place, so it no longer holds the value the variable was bound to and its
; debug user is killed.
;
; CHECK-LABEL: define i32 @dbg_value_in_place(
; CHECK-NEXT:    %mid = select i1 %B, i32 %x, i32 %z
; CHECK-NEXT:      #dbg_value(i32 poison, !{{[0-9]+}}, !DIExpression(), !{{[0-9]+}})
; CHECK-NEXT:    %outer = select i1 %A, i32 %mid, i32 %z
; CHECK-NEXT:    ret i32 %outer

define i32 @dbg_value_in_place(i1 %A, i1 %B, i32 %x, i32 %y, i32 %z) !dbg !4 {
  %inner = select i1 %A, i32 %x, i32 %y
  %mid   = select i1 %B, i32 %inner, i32 %z
  call void @llvm.dbg.value(metadata i32 %mid, metadata !9, metadata !DIExpression()), !dbg !10
  %outer = select i1 %A, i32 %mid, i32 %z
  ret i32 %outer
}

; The same function without the debug record - must not change pass folds.
;
; CHECK-LABEL: define i32 @same_shape_without_dbg(
; CHECK-NEXT:    %mid = select i1 %B, i32 %x, i32 %z
; CHECK-NEXT:    %outer = select i1 %A, i32 %mid, i32 %z
; CHECK-NEXT:    ret i32 %outer

define i32 @same_shape_without_dbg(i1 %A, i1 %B, i32 %x, i32 %y, i32 %z) {
  %inner = select i1 %A, i32 %x, i32 %y
  %mid   = select i1 %B, i32 %inner, i32 %z
  %outer = select i1 %A, i32 %mid, i32 %z
  ret i32 %outer
}

; %A proves %C, so %inner is bypassed rather than rewritten, and the orphan cleanup deletes it.
; A select is not salvageable, debug record is killed instead of rewriting it.
;
; CHECK-LABEL: define i32 @dbg_value_on_bypassed(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:      #dbg_value(i32 poison, !{{[0-9]+}}, !DIExpression(), !{{[0-9]+}})
; CHECK-NEXT:    %outer = select i1 %A, i32 %x, i32 %z
; CHECK-NEXT:    ret i32 %outer

define i32 @dbg_value_on_bypassed(i32 %n, i32 %x, i32 %y, i32 %z) !dbg !11 {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %inner = select i1 %C, i32 %x, i32 %y
  call void @llvm.dbg.value(metadata i32 %inner, metadata !12, metadata !DIExpression()), !dbg !13
  %outer = select i1 %A, i32 %inner, i32 %z
  ret i32 %outer
}

; %s has no real users at all - the debug record is its only reference. Both incoming values are %x,
; Dead-instruction cleanup deletes %s, and debug info is preserved.
;
; CHECK-LABEL: define i32 @dbg_value_only_user(
; CHECK-NEXT:      #dbg_value(i32 %x, !{{[0-9]+}}, !DIExpression(), !{{[0-9]+}})
; CHECK-NEXT:    ret i32 %w

define i32 @dbg_value_only_user(i1 %c, i32 %x, i32 %w) !dbg !14 {
  %s = select i1 %c, i32 %x, i32 %x
  call void @llvm.dbg.value(metadata i32 %s, metadata !15, metadata !DIExpression()), !dbg !16
  ret i32 %w
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}
!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "igc", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "t.cl", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "dbg_value_in_place", scope: !1, file: !1, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!5 = !DISubroutineType(types: !6)
!6 = !{!7}
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !DILocalVariable(name: "v", scope: !4, file: !1, line: 2, type: !7)
!10 = !DILocation(line: 2, column: 1, scope: !4)
!11 = distinct !DISubprogram(name: "dbg_value_on_bypassed", scope: !1, file: !1, line: 10, type: !5, scopeLine: 10, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!12 = !DILocalVariable(name: "w", scope: !11, file: !1, line: 11, type: !7)
!13 = !DILocation(line: 11, column: 1, scope: !11)
!14 = distinct !DISubprogram(name: "dbg_value_only_user", scope: !1, file: !1, line: 20, type: !5, scopeLine: 20, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!15 = !DILocalVariable(name: "u", scope: !14, file: !1, line: 21, type: !7)
!16 = !DILocation(line: 21, column: 1, scope: !14)
