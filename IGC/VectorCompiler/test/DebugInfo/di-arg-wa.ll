;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm_12_or_greater

; RUN: opt %use_old_pass_manager% -GenXDebugLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; Test checks DIArgList WA that replaces Arglist values with undef(if multiple).
;
; CHECK:  call void @llvm.dbg.value(metadata !DIArgList(i32 %a)
; CHECK-NEXT:  call void @llvm.dbg.value(metadata !DIArgList(i32 %b)
; CHECK-NEXT:  call void @llvm.dbg.value(metadata !DIArgList(i32 undef, i32 undef)

define spir_kernel void @test_arglist(i32 %a, i32 %b) !dbg !5 {
entry:
  call void @llvm.dbg.value(metadata !DIArgList(i32 %a), metadata !11, metadata !DIExpression(DW_OP_LLVM_arg, 0)), !dbg !12
  call void @llvm.dbg.value(metadata !DIArgList(i32 %b), metadata !13, metadata !DIExpression(DW_OP_LLVM_arg, 0)), !dbg !12
  call void @llvm.dbg.value(metadata !DIArgList(i32 %a, i32 %b), metadata !14, metadata !DIExpression(DW_OP_LLVM_arg, 0, DW_OP_LLVM_arg, 1, DW_OP_plus)), !dbg !12
  %add = add i32 %a, %b, !dbg !15
  call void @llvm.dbg.value(metadata i32 %add, metadata !9, metadata !DIExpression()), !dbg !15
  call void @use.i32(i32 %add), !dbg !16
  ret void, !dbg !17
}

declare void @use.i32(i32)

declare void @llvm.genx.GenISA.CatchAllDebugLine()

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{}
!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!2, !3}
!llvm.module.flags = !{!4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "1", directory: "/")
!2 = !{i32 3}
!3 = !{i32 1}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "test_arglist", linkageName: "test_arglist", scope: null, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{}
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !5, file: !1, line: 3, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "b", arg: 2, scope: !5, file: !1, line: 1, type: !10)
!12 = !DILocation(line: 0, column: 1, scope: !5)
!13 = !DILocalVariable(name: "a", arg: 1, scope: !5, file: !1, line: 1, type: !10)
!14 = !DILocalVariable(name: "c", scope: !5, file: !1, line: 2, type: !10)
!15 = !DILocation(line: 3, column: 1, scope: !5)
!16 = !DILocation(line: 4, column: 1, scope: !5)
!17 = !DILocation(line: 5, column: 1, scope: !5)
