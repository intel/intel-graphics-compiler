;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-break-const-expr -S < %s | FileCheck %s
; ------------------------------------------------
; BreakConstantExpr
; ------------------------------------------------
; This test checks that BreakConstantExpr pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'BreakConstantExpr.ll'
source_filename = "BreakConstantExpr.ll"

@x = addrspace(2) constant i32 3, align 4
@y = addrspace(1) constant i32 13, align 4
%st.type = type {i32, i64}

define spir_kernel void @test_breakexpr(i32* %dst, i32 %isrc, float %fsrc) !dbg !6 {
; Testcase 1:
; instruction operand constexpr break
; CHECK: [[EXTR_V:%[0-9]*]] = extractelement {{.*}} !dbg [[EXTR_LOC:![0-9]*]]
; CHECK-NEXT: dbg.value(metadata i32 [[EXTR_V]],  metadata [[EXTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR_LOC]]
  %1 = extractelement <2 x i32> <i32 add (i32 ptrtoint (i32 addrspace(2)* getelementptr inbounds (i32, i32 addrspace(2)* @x, i32 2) to i32), i32 1), i32 extractvalue( %st.type {i32 ptrtoint(i32 addrspace(1)* @y to i32), i64 13},  0)>, i32 1, !dbg !13
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !13
; Testcase 2:
; dbg.declare inrtinsic constexpr break
; CHECK: [[DCL_V:%[0-9]*]] = inttoptr i64 {{.*}} !dbg [[DCL_LOC:![0-9]*]]
; CHECK-NEXT: dbg.declare(metadata i64* [[DCL_V]],  metadata [[DCL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DCL_LOC]]
  call void @llvm.dbg.declare(metadata !{i64* inttoptr (i64 extractvalue( %st.type {i32 0, i64 ptrtoint(i32 addrspace(1)* @y to i64)},  1) to i64*)}, metadata !11, metadata !DIExpression()), !dbg !14
  ret void, !dbg !15
}

; Testcase 1 MD:
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[EXTR_MD]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)

; Testcase 2 MD:
; CHECK-DAG: [[DCL_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[DCL_MD]] = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "BreakConstantExpr.ll", directory: "/")
!2 = !{}
!3 = !{i32 3}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_breakexpr", linkageName: "test_breakexpr", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
