;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass
; ------------------------------------------------
; This test checks that CustomSafeOptPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_customsafe{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK: store i32 [[VAL1_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK-DAG: store float [[VAL10_V:%[A-z0-9]*]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL10_V]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]


define spir_kernel void @test_customsafe(i32 %a, i32* %b, float* %c) !dbg !6 {
entry:
  %0 = call i32 @llvm.genx.GenISA.bfi(i32 4, i32 8, i32 %a, i32 16), !dbg !20
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !20
  store i32 %0, i32* %b, !dbg !21
  %1 = bitcast i32 %a to float, !dbg !22
  call void @llvm.dbg.value(metadata float %1, metadata !11, metadata !DIExpression()), !dbg !22
  %2 = bitcast i32 %0 to float, !dbg !23
  call void @llvm.dbg.value(metadata float %2, metadata !12, metadata !DIExpression()), !dbg !23
  %3 = call float @llvm.genx.GenISA.f32tof16.rtz(float %1), !dbg !24
  call void @llvm.dbg.value(metadata float %3, metadata !13, metadata !DIExpression()), !dbg !24
  %4 = call float @llvm.genx.GenISA.f32tof16.rtz(float %2), !dbg !25
  call void @llvm.dbg.value(metadata float %4, metadata !14, metadata !DIExpression()), !dbg !25
  %5 = bitcast float %3 to i32, !dbg !26
  call void @llvm.dbg.value(metadata i32 %5, metadata !15, metadata !DIExpression()), !dbg !26
  %6 = bitcast float %4 to i32, !dbg !27
  call void @llvm.dbg.value(metadata i32 %6, metadata !16, metadata !DIExpression()), !dbg !27
  %7 = shl i32 %5, 16, !dbg !28
  call void @llvm.dbg.value(metadata i32 %7, metadata !17, metadata !DIExpression()), !dbg !28
  %8 = add i32 %6, %7, !dbg !29
  call void @llvm.dbg.value(metadata i32 %8, metadata !18, metadata !DIExpression()), !dbg !29
  %9 = bitcast i32 %8 to float, !dbg !30
  call void @llvm.dbg.value(metadata float %9, metadata !19, metadata !DIExpression()), !dbg !30
  store float %9, float* %c, !dbg !31
  ret void, !dbg !32
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "call-bfi-f32tof16.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

declare i32 @llvm.genx.GenISA.bfi(i32, i32, i32, i32)

declare float @llvm.genx.GenISA.f32tof16.rtz(float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "call-bfi-f32tof16.ll", directory: "/")
!2 = !{}
!3 = !{i32 13}
!4 = !{i32 10}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16, !17, !18, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !10)
!17 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 9, type: !10)
!18 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 10, type: !10)
!19 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 11, type: !10)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)
!28 = !DILocation(line: 9, column: 1, scope: !6)
!29 = !DILocation(line: 10, column: 1, scope: !6)
!30 = !DILocation(line: 11, column: 1, scope: !6)
!31 = !DILocation(line: 12, column: 1, scope: !6)
!32 = !DILocation(line: 13, column: 1, scope: !6)
