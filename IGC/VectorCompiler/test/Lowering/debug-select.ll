;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify update of debug metadata (select)

; CHECK: void @test_debug_select{{.*}}

define void @test_debug_select(i1 %a, i32* %b) {
entry:
  %0 = load i32, i32* %b
  %1 = bitcast i32 %0 to <32 x i1>
  %2 = select i1 %a, <32 x i1> %1, <32 x i1> bitcast (<1 x i32> <i32 13> to <32 x i1>), !dbg !19
  call void @llvm.dbg.value(metadata <32 x i1> %2, metadata !13, metadata !DIExpression()), !dbg !19
  ret void
}

; CHECK: br {{.*}} %select{{.*}} !dbg [[SEL_LOC:![0-9]*]]
; CHECK: br {{.*}} %select{{.*}} !dbg [[SEL_LOC]]
; CHECK: br {{.*}} %select{{.*}} !dbg [[SEL_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i1> [[SEL:%[0-9a-zA-Z\.]*]], metadata [[SEL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SEL_LOC]]
; CHECK-DAG: [[SEL]] = phi {{.*}} !dbg [[SEL_LOC]]

; CHECK-DAG: [[SEL_MD]] = !DILocalVariable(name:
; CHECK-DAG: [[SEL_LOC]] = !DILocation(line: 3, column: 1,

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug-select.ll", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug_select", linkageName: "test_debug_select", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!12 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!19 = !DILocation(line: 3, column: 1, scope: !6)
