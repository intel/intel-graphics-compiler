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
; CHECK-DAG: store float [[VAL4_V:%[A-z0-9]*]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: end:
; CHECK-DAG: store float [[VAL7_V:%[A-z0-9]*]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]

define spir_kernel void @test_customsafe(float %a, float %b, i1 %c, float* %d) !dbg !6 {
entry:
  %0 = bitcast float %a to i32, !dbg !17
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = bitcast float %b to i32, !dbg !18
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !18
  %2 = select i1 %c, i32 %0, i32 %1, !dbg !19
  call void @llvm.dbg.value(metadata i32 %2, metadata !12, metadata !DIExpression()), !dbg !19
  %3 = bitcast i32 %2 to float, !dbg !20
  call void @llvm.dbg.value(metadata float %3, metadata !13, metadata !DIExpression()), !dbg !20
  store float %3, float* %d, !dbg !21
  br i1 %c, label %bb1, label %end, !dbg !22

bb1:                                              ; preds = %entry
  %4 = bitcast float %a to <2 x i16>, !dbg !23
  call void @llvm.dbg.value(metadata <2 x i16> %4, metadata !14, metadata !DIExpression()), !dbg !23
  br label %end, !dbg !24

end:                                              ; preds = %bb1, %entry
  %5 = phi <2 x i16> [ <i16 0, i16 1>, %entry ], [ %4, %bb1 ], !dbg !25
  call void @llvm.dbg.value(metadata <2 x i16> %5, metadata !15, metadata !DIExpression()), !dbg !25
  %6 = bitcast <2 x i16> %5 to float, !dbg !26
  call void @llvm.dbg.value(metadata float %6, metadata !16, metadata !DIExpression()), !dbg !26
  store float %6, float* %d, !dbg !27
  ret void, !dbg !28
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "bitcast.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "bitcast.ll", directory: "/")
!2 = !{}
!3 = !{i32 12}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 9, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 10, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
!28 = !DILocation(line: 12, column: 1, scope: !6)
