;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify update of debug metadata (intrinsic)

; CHECK: void @test_debug_intrinsic{{.*}}

define void @test_debug_intrinsic(float* %a, <16 x float>* %b, <16 x float> %c, <16 x float*> %d) {
entry:
  ; T1 => llvm.fma.f32
  %0 = call float @llvm.fmuladd.f32(float 1.000000e+00, float 4.000000e+00, float 6.000000e+00), !dbg !14
  call void @llvm.dbg.value(metadata float %0, metadata !9, metadata !DIExpression()), !dbg !14

  ; T2 => ptrtoint+cast
  store float %0, float* %a, !dbg !15

  ret void
}

; CHECK-DAG: void @llvm.dbg.value(metadata float [[T1:%[0-9a-zA-Z\.]*]], metadata [[T1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[T1_LOC:![0-9]*]]
; CHECK-DAG: [[T1]] = call float @llvm.fma.f32{{.*}} !dbg [[T1_LOC]]

; CHECK: store float{{.*}}, !dbg [[T2_LOC:![0-9]*]]

; CHECK-DAG: [[T1_MD]] = !DILocalVariable(name:
; CHECK-DAG: [[T1_LOC]] = !DILocation(line: 1, column: 1,
; CHECK-DAG: [[T2_LOC]] = !DILocation(line: 2, column: 1,

declare float @llvm.fmuladd.f32(float, float, float) #0
declare void @llvm.masked.store.v16f32.p0v16f32(<16 x float>, <16 x float>*, i32 immarg, <16 x i1>) #1
declare <16 x float> @llvm.masked.load.v16f32.p0v16f32(<16 x float>*, i32 immarg, <16 x i1>, <16 x float>) #2

declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable willreturn }
attributes #1 = { argmemonly nounwind willreturn }
attributes #2 = { argmemonly nounwind readonly willreturn }
attributes #3 = { nounwind readonly willreturn }
attributes #4 = { nounwind willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug-intrinsic.ll", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug_intrinsic", linkageName: "test_debug_intrinsic", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !12)
!14 = !DILocation(line: 1, column: 1, scope: !6)
!15 = !DILocation(line: 2, column: 1, scope: !6)
!16 = !DILocation(line: 3, column: 1, scope: !6)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
!19 = !DILocation(line: 6, column: 1, scope: !6)
