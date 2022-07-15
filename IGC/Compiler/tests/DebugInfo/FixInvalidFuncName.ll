;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -fix-invalid-func-name -S < %s | FileCheck %s
; ------------------------------------------------
; FixInvalidFuncName
; ------------------------------------------------
; This test checks that FixInvalidFuncName pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: store i64 [[VAL1_V]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]

define spir_kernel void @test(i64* %dst) !dbg !6 {
  %1 = call spir_func i64 @"in$li.d_func"(i64* %dst), !dbg !11
  call void @llvm.dbg.value(metadata i64 %1, metadata !9, metadata !DIExpression()), !dbg !11
  store i64 %1, i64* %dst, !dbg !12
  ret void, !dbg !13
}

; CHECK: define spir_func i64 {{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]

define spir_func i64 @"in$li.d_func"(i64* %src) !dbg !14 {
  %1 = load i64, i64* %src, !dbg !17
  call void @llvm.dbg.value(metadata i64 %1, metadata !16, metadata !DIExpression()), !dbg !17
  ret i64 %1, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "FixInvalidFuncName.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "in$li.d_func", linkageName: "in$li.d_func", scope: null, file: [[FILE]], line: 4
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE2]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE2]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "FixInvalidFuncName.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!14 = distinct !DISubprogram(name: "in$li.d_func", linkageName: "in$li.d_func", scope: null, file: !1, line: 4, type: !7, scopeLine: 4, unit: !0, retainedNodes: !15)
!15 = !{!16}
!16 = !DILocalVariable(name: "2", scope: !14, file: !1, line: 4, type: !10)
!17 = !DILocation(line: 4, column: 1, scope: !14)
!18 = !DILocation(line: 5, column: 1, scope: !14)
