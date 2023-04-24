;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXCoalescingWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXCoalescing
; ------------------------------------------------
; This test checks that GenXCoalescing pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; Check global load coalescing

; CHECK: define dllexport void @test_coal{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <128 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <128 x i32> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: store {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

@b = common global <128 x i32> zeroinitializer #0

define dllexport void @test_coal(<128 x i32>* %a) #1 !dbg !12 {
entry:
  %0 = load <128 x i32>, <128 x i32>* @b, !dbg !18
  call void @llvm.dbg.value(metadata <128 x i32> %0, metadata !15, metadata !DIExpression()), !dbg !18
  %1 = load <128 x i32>, <128 x i32>* @b, !dbg !19
  call void @llvm.dbg.value(metadata <128 x i32> %1, metadata !17, metadata !DIExpression()), !dbg !19
  store <128 x i32> %0, <128 x i32>* %a, !dbg !20
  store <128 x i32> %1, <128 x i32>* %a, !dbg !21
  ret void, !dbg !22
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "coal-gl.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_coal", linkageName: "test_coal", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

attributes #0 = { "genx_volatile" }
attributes #1 = { "CMGenxMain" }
attributes #2 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}
!llvm.dbg.cu = !{!6}
!llvm.debugify = !{!9, !10}
!llvm.module.flags = !{!11}

!0 = !{void (<128 x i32>*)* @test_coal, !"test_coal", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2, i32 2}
!2 = !{i32 1, i32 13}
!3 = !{i32 42, i32 2}
!4 = !{!"buffer_t"}
!5 = !{void (<128 x i32>*)* @test_coal, null, null, null, null}
!6 = distinct !DICompileUnit(language: DW_LANG_C, file: !7, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !8)
!7 = !DIFile(filename: "coal-gl.ll", directory: "/")
!8 = !{}
!9 = !{i32 5}
!10 = !{i32 2}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = distinct !DISubprogram(name: "test_coal", linkageName: "test_coal", scope: null, file: !7, line: 1, type: !13, scopeLine: 1, unit: !6, retainedNodes: !14)
!13 = !DISubroutineType(types: !8)
!14 = !{!15, !17}
!15 = !DILocalVariable(name: "1", scope: !12, file: !7, line: 1, type: !16)
!16 = !DIBasicType(name: "ty4096", size: 4096, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "2", scope: !12, file: !7, line: 2, type: !16)
!18 = !DILocation(line: 1, column: 1, scope: !12)
!19 = !DILocation(line: 2, column: 1, scope: !12)
!20 = !DILocation(line: 3, column: 1, scope: !12)
!21 = !DILocation(line: 4, column: 1, scope: !12)
!22 = !DILocation(line: 5, column: 1, scope: !12)
