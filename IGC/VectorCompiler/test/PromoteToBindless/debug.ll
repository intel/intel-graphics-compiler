;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPromoteStatefulToBindless
; ------------------------------------------------
; This test checks that GenXPromoteStatefulToBindless pass follows
; 'How to Update Debug Info' llvm guideline.


; CHECK: spir_kernel void @promote{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <16 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: oword{{.*}} !dbg [[STR1_LOC:![0-9]*]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <16 x i1> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <64 x i32> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: lsc.store{{.*}} !dbg [[STR2_LOC:![0-9]*]]

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define dllexport spir_kernel void @promote(i32 %s1, i32 %s2) #0 !dbg !11 {
  %1 = call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 %s1, i32 0), !dbg !20
  call void @llvm.dbg.value(metadata <16 x i32> %1, metadata !14, metadata !DIExpression()), !dbg !20
  call void @llvm.genx.oword.st.v16i32(i32 %s1, i32 0, <16 x i32> %1), !dbg !21
  %2 = trunc <16 x i32> %1 to <16 x i1>, !dbg !22
  call void @llvm.dbg.value(metadata <16 x i1> %2, metadata !16, metadata !DIExpression()), !dbg !22
  %3 = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v16i1.v16i32(<16 x i1> %2, i8 0, i8 1, i8 2, i16 3, i32 4, i8 5, i8 6, i8 7, i8 8, <16 x i32> %1, i32 %s1), !dbg !23
  call void @llvm.dbg.value(metadata <64 x i32> %3, metadata !18, metadata !DIExpression()), !dbg !23
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1> %2, i8 0, i8 1, i8 1, i16 2, i32 3, i8 4, i8 5, i8 6, i8 7, <16 x i32> %1, <64 x i32> %3, i32 %s1), !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPromoteStatefulToBindless.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "promote", linkageName: "promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])


declare <16 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32)

declare void @llvm.genx.oword.st.v16i32(i32, i32, <16 x i32>)

declare <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <64 x i32>, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}
!llvm.dbg.cu = !{!5}
!llvm.debugify = !{!8, !9}
!llvm.module.flags = !{!10}

!0 = !{void (i32, i32)* @promote, !"promote", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 1}
!2 = !{i32 0, i32 0}
!3 = !{!"buffer_t read_write", !"sampler_t"}
!4 = !{void (i32, i32)* @promote, null, null, null, null}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !7)
!6 = !DIFile(filename: "GenXPromoteStatefulToBindless.ll", directory: "/")
!7 = !{}
!8 = !{i32 6}
!9 = !{i32 3}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = distinct !DISubprogram(name: "promote", linkageName: "promote", scope: null, file: !6, line: 1, type: !12, scopeLine: 1, unit: !5, retainedNodes: !13)
!12 = !DISubroutineType(types: !7)
!13 = !{!14, !16, !18}
!14 = !DILocalVariable(name: "1", scope: !11, file: !6, line: 1, type: !15)
!15 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "2", scope: !11, file: !6, line: 3, type: !17)
!17 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "3", scope: !11, file: !6, line: 4, type: !19)
!19 = !DIBasicType(name: "ty2048", size: 2048, encoding: DW_ATE_unsigned)
!20 = !DILocation(line: 1, column: 1, scope: !11)
!21 = !DILocation(line: 2, column: 1, scope: !11)
!22 = !DILocation(line: 3, column: 1, scope: !11)
!23 = !DILocation(line: 4, column: 1, scope: !11)
!24 = !DILocation(line: 5, column: 1, scope: !11)
!25 = !DILocation(line: 6, column: 1, scope: !11)
