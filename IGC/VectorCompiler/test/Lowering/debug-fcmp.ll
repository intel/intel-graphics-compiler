;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify update of debug metadata (fcmp)

; CHECK: i1 @test_debug_fcmp{{.*}}

define i1 @test_debug_fcmp(float %a, float %b) {
entry:
  %0 = fcmp ult float %a, %b, !dbg !11
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !11
  ret i1 %0
}

; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[FCMP:%[0-9a-zA-Z\.]*]], metadata [[FCMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_LOC:![0-9]*]]
; CHECK-DAG: [[INVERSE:%[0-9a-zA-Z\.]*]] = fcmp oge {{.*}} !dbg [[FCMP_LOC]]
; CHECK-DAG: [[FCMP]] = xor i1 [[INVERSE]], {{.*}} !dbg [[FCMP_LOC]]

; CHECK-DAG: [[FCMP_MD]] = !DILocalVariable(name:
; CHECK-DAG: [[FCMP_LOC]] = !DILocation(line: 1, column: 1,

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug-fcmp.ll", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug_fcmp", linkageName: "test_debug_fcmp", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
