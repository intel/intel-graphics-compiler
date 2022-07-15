;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-genrotate -platformdg1 -S < %s | FileCheck %s
; ------------------------------------------------
; GenRotate
; ------------------------------------------------
; This test checks that GenRotate pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_rotate
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: [[SHL_V:%[0-9]*]] = shl
; CHECK-SAME: !dbg [[SHL_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[SHL_V]]
; CHECK-SAME: metadata [[SHL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHL_LOC]]
; CHECK: [[SHR_V:%[0-9]*]] = lshr
; CHECK-SAME: !dbg [[SHR_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[SHR_V]]
; CHECK-SAME: metadata [[SHR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHR_LOC]]
; CHECK: [[ROL_V:%[A-z0-9]*]] =
; CHECK-SAME: !dbg [[ROL_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[ROL_V]]
; CHECK-SAME: metadata [[ROL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ROL_LOC]]
; CHECK: store
; CHECK-SAME: !dbg [[STORE_LOC:![0-9]*]]

define void @test_rotate(i32 %src, i32* %dst) !dbg !6 {
  %1 = shl i32 %src, 14, !dbg !13
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !13
  %2 = lshr i32 %src, 18, !dbg !14
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !14
  %3 = or i32 %1, %2, !dbg !15
  call void @llvm.dbg.value(metadata i32 %3, metadata !12, metadata !DIExpression()), !dbg !15
  store i32 %3, i32* %dst, !dbg !16
  ret void, !dbg !17
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenRotate.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_rotate", linkageName: "test_rotate", scope: null, file:  [[FILE]], line: 1
; CHECK-DAG: [[SHL_MD]] = !DILocalVariable(name: "1", scope:  [[SCOPE]], file:  [[FILE]], line: 1
; CHECK-DAG: [[SHL_LOC]] = !DILocation(line: 1, column: 1, scope:  [[SCOPE]])
; CHECK-DAG: [[SHR_MD]] = !DILocalVariable(name: "2", scope:  [[SCOPE]], file:  [[FILE]], line: 2
; CHECK-DAG: [[SHR_LOC]] = !DILocation(line: 2, column: 1, scope:  [[SCOPE]])
; CHECK-DAG: [[ROL_MD]] = !DILocalVariable(name: "3", scope:  [[SCOPE]], file:  [[FILE]], line: 3
; CHECK-DAG: [[ROL_LOC]] = !DILocation(line: 3, column: 1, scope:  [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 4, column: 1, scope:  [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenRotate.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_rotate", linkageName: "test_rotate", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
