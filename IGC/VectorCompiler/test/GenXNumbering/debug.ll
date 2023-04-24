;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXNumberingWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXNumbering
; ------------------------------------------------
; This test checks that GenXNumbering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; This pass is essentialy noop from debug perspective (sets some internal data),
; doesn't modify anything, so its a sanity check that nothing was lost

; CHECK: define dllexport void @test_kernel{{.*}} !dbg [[KSCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: call void @test_func{{.*}}, !dbg [[CALL1_LOC:![0-9]*]]

define dllexport void @test_kernel(i32* %a) #0 !dbg !6 {
  %1 = load i32, i32* %a, !dbg !13
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !13
  %2 = inttoptr i32 %1 to i32*, !dbg !14
  call void @llvm.dbg.value(metadata i32* %2, metadata !11, metadata !DIExpression()), !dbg !14
  call void @test_func(i32* %2), !dbg !15
  ret void, !dbg !16
}

; CHECK: define internal void @test_func{{.*}} !dbg [[FSCOPE:![0-9]*]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

; Function Attrs: noinline
define internal void @test_func(i32* %a) #1 !dbg !17 {
  %1 = load i32, i32* %a, !dbg !21
  call void @llvm.dbg.value(metadata i32 %1, metadata !19, metadata !DIExpression()), !dbg !21
  %2 = add i32 %1, 13, !dbg !22
  call void @llvm.dbg.value(metadata i32 %2, metadata !20, metadata !DIExpression()), !dbg !22
  store i32 %2, i32* %a, !dbg !23
  ret void, !dbg !24
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXNumbering.ll", directory: "/")
; CHECK-DAG: [[KSCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[KSCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[KSCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[KSCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[KSCOPE]])
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 3, column: 1, scope: [[KSCOPE]])

; CHECK-DAG: [[FSCOPE]] = distinct !DISubprogram(name: "test_func", linkageName: "test_func", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[FSCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[FSCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[FSCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1, scope: [[FSCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 7, column: 1, scope: [[FSCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

attributes #0 = { "CMGenxMain" }
attributes #1 = { noinline }
attributes #2 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXNumbering.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = distinct !DISubprogram(name: "test_func", linkageName: "test_func", scope: null, file: !1, line: 5, type: !7, scopeLine: 5, unit: !0, retainedNodes: !18)
!18 = !{!19, !20}
!19 = !DILocalVariable(name: "3", scope: !17, file: !1, line: 5, type: !10)
!20 = !DILocalVariable(name: "4", scope: !17, file: !1, line: 6, type: !10)
!21 = !DILocation(line: 5, column: 1, scope: !17)
!22 = !DILocation(line: 6, column: 1, scope: !17)
!23 = !DILocation(line: 7, column: 1, scope: !17)
!24 = !DILocation(line: 8, column: 1, scope: !17)
