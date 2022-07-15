;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-legalize-function-signatures -S < %s | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------
; This test checks that LegalizeFunctionSignatures pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

%str = type { i32, i64 }

;
; Testcase 1:
; Calls are substituted with calls to legalized functions
;
; CHECK: [[FOO_V:%[0-9]*]] = call i32 @foo({{.*}} [[FOO_ARG:%[0-9]*]]), !dbg [[FOO_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[FOO_V]], metadata [[FOO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FOO_LOC]]
; CHECK: call void @bar{{.*}}, !dbg [[BAR_LOC:![0-9]*]]
;

define spir_kernel void @test_k(i32 %src) !dbg !6 {
  %1 = insertvalue %str { i32 0, i64 32 }, i32 %src, 0, !dbg !16
  call void @llvm.dbg.value(metadata %str %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = call i32 @foo(%str %1), !dbg !17
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = insertvalue [2 x i32] [i32 0, i32 32], i32 %src, 0, !dbg !18
  call void @llvm.dbg.value(metadata [2 x i32] %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = insertvalue [2 x i32] %3, i32 %2, 1, !dbg !19
  call void @llvm.dbg.value(metadata [2 x i32] %4, metadata !15, metadata !DIExpression()), !dbg !19
  call void @bar([2 x i32] %3), !dbg !20
  ret void, !dbg !21
}

; Testcase 2:
;
; Substituted function has it's debug preserved.
; CHECK: define spir_func i32 @foo({{.*}}) !dbg [[FOO_SCOPE:![0-9]*]]
; CHECK: [[EXTR_V:%[0-9]*]] = extractvalue %str [[ARG_V:%[A-z0-9]*]], 0, !dbg [[EXTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[EXTR_V]], metadata [[EXTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR_LOC]]

define spir_func i32 @foo(%str %src) !dbg !22 {
  %1 = extractvalue %str %src, 0, !dbg !25
  call void @llvm.dbg.value(metadata i32 %1, metadata !24, metadata !DIExpression()), !dbg !25
  ret i32 %1, !dbg !26
}

; MD checks:
;
; CHECK-DAG: [[KERNEL_SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_k", linkageName: "test_k", scope: null
; CHECK-DAG: [[FOO_LOC]] = !DILocation(line: 2, column: 1, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[FOO_MD]] = !DILocalVariable(name: "2", scope: [[KERNEL_SCOPE]], file: {{.*}}, line: 2
; CHECK-DAG: [[BAR_LOC]] = !DILocation(line: 5, column: 1, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[FOO_SCOPE]] =  distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: {{.*}}, line: 7
; CHECK-DAG: [[EXTR_MD]] =  !DILocalVariable(name: "5", scope: [[FOO_SCOPE]], file: {{.*}}, line: 7
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 7, column: 1, scope: [[FOO_SCOPE]])

declare void @bar([2 x i32]) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "IndirectlyCalled" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "LegalizeFunctionSignatures/fixargs.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_k", linkageName: "test_k", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty96", size: 96, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 7, type: !7, scopeLine: 7, unit: !0, retainedNodes: !23)
!23 = !{!24}
!24 = !DILocalVariable(name: "5", scope: !22, file: !1, line: 7, type: !12)
!25 = !DILocation(line: 7, column: 1, scope: !22)
!26 = !DILocation(line: 8, column: 1, scope: !22)
