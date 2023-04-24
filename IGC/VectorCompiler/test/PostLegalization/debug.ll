;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPostLegalization
; ------------------------------------------------
; This test checks that GenXPostLegalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: <16 x i32> @test_postlegalize{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL1_V:%[A-z0-9.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK: store {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

define <16 x i32> @test_postlegalize(<16 x i32> %a, <16 x i32>* %b) !dbg !6 {
  %1 = and <16 x i32> %a, <i32 1, i32 2, i32 3, i32 4, i32 5, i32 66, i32 77, i32 88, i32 99, i32 -100, i32 111, i32 122, i32 133, i32 4000, i32 4500, i32 122123>, !dbg !11
  call void @llvm.dbg.value(metadata <16 x i32> %1, metadata !9, metadata !DIExpression()), !dbg !11
  store <16 x i32> %1, <16 x i32>* %b, !dbg !12
  ret <16 x i32> %1, !dbg !13
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_postlegalize", linkageName: "test_postlegalize", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "basic.ll", directory: "/")
!2 = !{}
!3 = !{i32 3}
!4 = !{i32 1}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_postlegalize", linkageName: "test_postlegalize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
