;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -S < %s | FileCheck %s --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-DBG-RECORDS%} %else %{CHECK-DBG-INTRINSIC%}
; ------------------------------------------------
; CustomSafeOptPass -- constant-folded compares
; ------------------------------------------------
; Both visitXor and visitAnd redirect the original value's debug variables onto
; a value that is its inverse, so the DIExpressions must negate it back with
; DW_OP_constu 1, DW_OP_xor, DW_OP_stack_value.
;
; When every compare operand is constant, IRBuilder folds the inverted compare
; (and, in visitAnd, the or) to a constant. The DIExpression correction is still
; required in that case; without it the debugger reports the negated value.
; ------------------------------------------------

declare void @use.i32(i32)

; visitXor, folded: `icmp eq 0, 0` is true, its inverse folds to `false`, and the
; expression must xor that back to true.
define spir_kernel void @test_xor_const_operands() !dbg !4 {
; CHECK: define spir_kernel void @test_xor_const_operands{{.*}} !dbg
; CHECK-NOT: = icmp
; CHECK-NOT: = xor
; CHECK-DBG-INTRINSIC: void @llvm.dbg.value(metadata i1 false, metadata [[XOR_MD:![0-9]*]], metadata !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value))
; CHECK-DBG-RECORDS: #dbg_value(i1 false, [[XOR_MD:![0-9]*]], !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value)
; CHECK: br i1 false
entry:
  %cmp = icmp eq i32 0, 0, !dbg !10
  call void @llvm.dbg.value(metadata i1 %cmp, metadata !9, metadata !DIExpression()), !dbg !10
  %not = xor i1 %cmp, true, !dbg !10
  br i1 %not, label %bb1, label %bb2, !dbg !10
bb1:
  call void @use.i32(i32 11), !dbg !10
  ret void, !dbg !10
bb2:
  call void @use.i32(i32 22), !dbg !10
  ret void, !dbg !10
}

; visitXor, not folded: a real inverted compare is created, and the same
; expression correction applies. This is the control for the case above.
define spir_kernel void @test_xor_nonconst_operands(i32 %x, i32 %y) !dbg !11 {
; CHECK: define spir_kernel void @test_xor_nonconst_operands{{.*}} !dbg
; CHECK: [[INV:%[A-z0-9.]*]] = icmp ne i32 %x, %y
; CHECK-NOT: = xor
; CHECK-DBG-INTRINSIC: void @llvm.dbg.value(metadata i1 [[INV]], metadata [[NC_MD:![0-9]*]], metadata !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value))
; CHECK-DBG-RECORDS: #dbg_value(i1 [[INV]], [[NC_MD:![0-9]*]], !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value)
entry:
  %cmp = icmp eq i32 %x, %y, !dbg !13
  call void @llvm.dbg.value(metadata i1 %cmp, metadata !12, metadata !DIExpression()), !dbg !13
  %not = xor i1 %cmp, true, !dbg !13
  br i1 %not, label %bb1, label %bb2, !dbg !13
bb1:
  call void @use.i32(i32 11), !dbg !13
  ret void, !dbg !13
bb2:
  call void @use.i32(i32 22), !dbg !13
  ret void, !dbg !13
}

; visitAnd, folded: `and (xor true, 1), (icmp eq 0, 0)` is `and false, true` =
; false, and the rewritten `or` folds to true, so the expression must xor it
; back to false.
define spir_kernel void @test_and_const_operands() !dbg !14 {
; CHECK: define spir_kernel void @test_and_const_operands{{.*}} !dbg
; CHECK-NOT: = and
; CHECK-DBG-INTRINSIC: void @llvm.dbg.value(metadata i1 true, metadata [[AND_MD:![0-9]*]], metadata !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value))
; CHECK-DBG-RECORDS: #dbg_value(i1 true, [[AND_MD:![0-9]*]], !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value)
; CHECK: br i1 true
entry:
  %xnot = xor i1 true, true, !dbg !16
  %cmp = icmp eq i32 0, 0, !dbg !16
  %and = and i1 %xnot, %cmp, !dbg !16
  call void @llvm.dbg.value(metadata i1 %and, metadata !15, metadata !DIExpression()), !dbg !16
  br i1 %and, label %bb1, label %bb2, !dbg !16
bb1:
  call void @use.i32(i32 11), !dbg !16
  ret void, !dbg !16
bb2:
  call void @use.i32(i32 22), !dbg !16
  ret void, !dbg !16
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "igc", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "xor-const-operands.ll", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "test_xor_const_operands", scope: !1, file: !1, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!5 = !DISubroutineType(types: !2)
!8 = !DIBasicType(name: "bool", size: 8, encoding: DW_ATE_boolean)
!9 = !DILocalVariable(name: "c", scope: !4, file: !1, line: 2, type: !8)
!10 = !DILocation(line: 2, column: 1, scope: !4)
!11 = distinct !DISubprogram(name: "test_xor_nonconst_operands", scope: !1, file: !1, line: 10, type: !5, scopeLine: 10, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!12 = !DILocalVariable(name: "c", scope: !11, file: !1, line: 11, type: !8)
!13 = !DILocation(line: 11, column: 1, scope: !11)
!14 = distinct !DISubprogram(name: "test_and_const_operands", scope: !1, file: !1, line: 20, type: !5, scopeLine: 20, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!15 = !DILocalVariable(name: "a", scope: !14, file: !1, line: 21, type: !8)
!16 = !DILocation(line: 21, column: 1, scope: !14)
