;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -cmkernelargoffset -cmkernelargoffset-cmrt=false -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; CMKernelArgOffset
; ------------------------------------------------
; This test checks that CMKernelArgOffset pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; This pass updates MD, and some used values, check that debug MD is not affected.

; CHECK: void @test_kernelargs{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = getelementptr inbounds %struct.st, {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32>* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: store i32 {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store i32 {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: store <4 x i32> {{.*}}, !dbg [[STORE3_LOC:![0-9]*]]

%struct.st = type { i32, <4 x i32>, [2 x i32] }

define void @test_kernelargs(%struct.st* byval(%struct.st) %st, i32 addrspace(1)* %a, <4 x i32> %b) !dbg !14 {
  %1 = getelementptr inbounds %struct.st, %struct.st* %st, i64 0, i32 0, !dbg !23
  call void @llvm.dbg.value(metadata i32* %1, metadata !17, metadata !DIExpression()), !dbg !23
  %2 = getelementptr inbounds %struct.st, %struct.st* %st, i64 0, i32 2, i32 0, !dbg !24
  call void @llvm.dbg.value(metadata i32* %2, metadata !19, metadata !DIExpression()), !dbg !24
  %3 = getelementptr inbounds %struct.st, %struct.st* %st, i64 0, i32 1, !dbg !25
  call void @llvm.dbg.value(metadata <4 x i32>* %3, metadata !20, metadata !DIExpression()), !dbg !25
  %4 = load i32, i32 addrspace(1)* %a, !dbg !26
  call void @llvm.dbg.value(metadata i32 %4, metadata !21, metadata !DIExpression()), !dbg !26
  store i32 %4, i32* %1, !dbg !27
  store i32 %4, i32* %2, !dbg !28
  store <4 x i32> %b, <4 x i32>* %3, !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "CMKernelArgOffset.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernelargs", linkageName: "test_kernelargs", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}
!llvm.dbg.cu = !{!9}
!llvm.debugify = !{!11, !12}
!llvm.module.flags = !{!13}

!0 = !{void (%struct.st*, i32 addrspace(1)*, <4 x i32>)* @test_kernelargs, !"test_kernelargs", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 112, i32 0, i32 96}
!2 = !{i32 0, i32 0}
!3 = !{}
!4 = !{void (%struct.st*, i32 addrspace(1)*, <4 x i32>)* @test_kernelargs, null, null, !5, null}
!5 = !{!6}
!6 = !{i32 0, !7}
!7 = !{!8}
!8 = !{i32 2, i32 0}
!9 = distinct !DICompileUnit(language: DW_LANG_C, file: !10, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !3)
!10 = !DIFile(filename: "CMKernelArgOffset.ll", directory: "/")
!11 = !{i32 8}
!12 = !{i32 4}
!13 = !{i32 2, !"Debug Info Version", i32 3}
!14 = distinct !DISubprogram(name: "test_kernelargs", linkageName: "test_kernelargs", scope: null, file: !10, line: 1, type: !15, scopeLine: 1, unit: !9, retainedNodes: !16)
!15 = !DISubroutineType(types: !3)
!16 = !{!17, !19, !20, !21}
!17 = !DILocalVariable(name: "1", scope: !14, file: !10, line: 1, type: !18)
!18 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "2", scope: !14, file: !10, line: 2, type: !18)
!20 = !DILocalVariable(name: "3", scope: !14, file: !10, line: 3, type: !18)
!21 = !DILocalVariable(name: "4", scope: !14, file: !10, line: 4, type: !22)
!22 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!23 = !DILocation(line: 1, column: 1, scope: !14)
!24 = !DILocation(line: 2, column: 1, scope: !14)
!25 = !DILocation(line: 3, column: 1, scope: !14)
!26 = !DILocation(line: 4, column: 1, scope: !14)
!27 = !DILocation(line: 5, column: 1, scope: !14)
!28 = !DILocation(line: 6, column: 1, scope: !14)
!29 = !DILocation(line: 7, column: 1, scope: !14)
!30 = !DILocation(line: 8, column: 1, scope: !14)
