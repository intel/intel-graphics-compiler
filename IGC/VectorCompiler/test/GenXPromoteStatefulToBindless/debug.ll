;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -vc-use-bindless-buffers -GenXPromoteStatefulToBindless  -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPromoteStatefulToBindless
; ------------------------------------------------
; This test checks that GenXPromoteStatefulToBindless pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: void @test_promote{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = call <8 x i32> {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <8 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: oword.st{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

define dllexport spir_kernel void @test_promote(i32 %surf, i32 %local_size) !dbg !11 {
entry:
  %0 = call <8 x i32> @llvm.genx.lsc.load.bti.v8i32.v8i1.v8i32(<8 x i1> zeroinitializer, i8 0, i8 0, i8 0, i16 0, i32 0, i8 0, i8 0, i8 0, i8 0, <8 x i32> zeroinitializer, i32 %surf), !dbg !16
  call void @llvm.dbg.value(metadata <8 x i32> %0, metadata !14, metadata !DIExpression()), !dbg !16
  call void @llvm.genx.oword.st.v8i32(i32 %surf, i32 0, <8 x i32> %0), !dbg !17
  ret void, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPromoteStatefulToBindless.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

declare <8 x i32> @llvm.genx.lsc.load.bti.v8i32.v8i1.v8i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, i32)

declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}
!llvm.dbg.cu = !{!5}
!llvm.debugify = !{!8, !9}
!llvm.module.flags = !{!10}

!0 = !{void (i32, i32)* @test_promote, !"test_promote", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 244}
!2 = !{i32 42}
!3 = !{!"buffer_t"}
!4 = !{void (i32, i32)* @test_promote, null, null, null, null}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !7)
!6 = !DIFile(filename: "GenXPromoteStatefulToBindless.ll", directory: "/")
!7 = !{}
!8 = !{i32 3}
!9 = !{i32 1}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !6, line: 1, type: !12, scopeLine: 1, unit: !5, retainedNodes: !13)
!12 = !DISubroutineType(types: !7)
!13 = !{!14}
!14 = !DILocalVariable(name: "1", scope: !11, file: !6, line: 1, type: !15)
!15 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !11)
!17 = !DILocation(line: 2, column: 1, scope: !11)
!18 = !DILocation(line: 3, column: 1, scope: !11)
