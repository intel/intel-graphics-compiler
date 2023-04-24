;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXDebugLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify removal of address space debug metadata (DW_OP_const/DW_OP_swap/DW_OP_xderef)

; CHECK: i32 @test_debug_addrspace{{.*}}

define i32 @test_debug_addrspace(i32 %0, i32 %1) {
entry:
  call void @llvm.dbg.value(metadata i32 %0, metadata !21, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !35
  call void @llvm.dbg.value(metadata i32 %1, metadata !22, metadata !DIExpression(DW_OP_plus_uconst, 50, DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef, DW_OP_stack_value)), !dbg !35
  %rv = add i32 %0, %1
  ret i32 %rv
}


; CHECK: void @llvm.dbg.value(metadata i32 %0, metadata ![[MD_NUM:[0-9]+]], metadata !DIExpression())
; CHECK: void @llvm.dbg.value(metadata i32 %1, metadata ![[MD2_NUM:[0-9]+]], metadata !DIExpression(DW_OP_plus_uconst, 50, DW_OP_stack_value))
; CHECK: %rv = add i32 %0, %1
; CHECK: [[MD_NUM]] = !DILocalVariable(name: "i1",
; CHECK: [[MD2_NUM]] = !DILocalVariable(name: "i2",

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "address-space.ll", directory: "/")
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug_addrspace", linkageName: "test_debug_addrspace", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!16 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "i1", scope: !6, file: !1, line: 11, type: !16)
!22 = !DILocalVariable(name: "i2", scope: !6, file: !1, line: 11, type: !16)
!35 = !DILocation(line: 11, column: 1, scope: !6)
