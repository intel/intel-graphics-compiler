;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -IGCIndirectICBPropagaion -S < %s | FileCheck %s
; ------------------------------------------------
; IGCIndirectICBPropagaion
; ------------------------------------------------
; This test checks that IGCIndirectICBPropagaion pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_indirectprop{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(65549)* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK: store i32 [[VAL2_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float addrspace(65549)* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK: store float [[VAL4_V]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

define void @test_indirectprop(i32 %a, i32* %b, float* %c) !dbg !20 {
entry:
  %0 = inttoptr i32 %a to i32 addrspace(65549)*, !dbg !29
  call void @llvm.dbg.value(metadata i32 addrspace(65549)* %0, metadata !23, metadata !DIExpression()), !dbg !29
  %1 = load i32, i32 addrspace(65549)* %0, !dbg !30
  call void @llvm.dbg.value(metadata i32 %1, metadata !25, metadata !DIExpression()), !dbg !30
  store i32 %1, i32* %b, !dbg !31
  %2 = inttoptr i32 %a to float addrspace(65549)*, !dbg !32
  call void @llvm.dbg.value(metadata float addrspace(65549)* %2, metadata !27, metadata !DIExpression()), !dbg !32
  %3 = load float, float addrspace(65549)* %2, !dbg !33
  call void @llvm.dbg.value(metadata float %3, metadata !28, metadata !DIExpression()), !dbg !33
  store float %3, float* %c, !dbg !34
  ret void, !dbg !35
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "IGCIndirectICBPropagaion.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_indirectprop", linkageName: "test_indirectprop", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!14}
!llvm.debugify = !{!17, !18}
!llvm.module.flags = !{!19}

!0 = !{!"ModuleMD", !1, !11}
!1 = !{!"immConstant", !2}
!2 = !{!"data", !3, !4, !5, !6, !7, !8, !9, !10}
!3 = !{!"dataVec[0]", i8 1}
!4 = !{!"dataVec[1]", i8 2}
!5 = !{!"dataVec[2]", i8 3}
!6 = !{!"dataVec[3]", i8 4}
!7 = !{!"dataVec[4]", i8 5}
!8 = !{!"dataVec[5]", i8 6}
!9 = !{!"dataVec[6]", i8 7}
!10 = !{!"dataVec[7]", i8 8}
!11 = !{!"pushInfo", !12, !13}
!12 = !{!"inlineConstantBufferOffset", i32 -1}
!13 = !{!"inlineConstantBufferSlot", i32 13}
!14 = distinct !DICompileUnit(language: DW_LANG_C, file: !15, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !16)
!15 = !DIFile(filename: "IGCIndirectICBPropagaion.ll", directory: "/")
!16 = !{}
!17 = !{i32 7}
!18 = !{i32 4}
!19 = !{i32 2, !"Debug Info Version", i32 3}
!20 = distinct !DISubprogram(name: "test_indirectprop", linkageName: "test_indirectprop", scope: null, file: !15, line: 1, type: !21, scopeLine: 1, unit: !14, retainedNodes: !22)
!21 = !DISubroutineType(types: !16)
!22 = !{!23, !25, !27, !28}
!23 = !DILocalVariable(name: "1", scope: !20, file: !15, line: 1, type: !24)
!24 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!25 = !DILocalVariable(name: "2", scope: !20, file: !15, line: 2, type: !26)
!26 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!27 = !DILocalVariable(name: "3", scope: !20, file: !15, line: 4, type: !24)
!28 = !DILocalVariable(name: "4", scope: !20, file: !15, line: 5, type: !26)
!29 = !DILocation(line: 1, column: 1, scope: !20)
!30 = !DILocation(line: 2, column: 1, scope: !20)
!31 = !DILocation(line: 3, column: 1, scope: !20)
!32 = !DILocation(line: 4, column: 1, scope: !20)
!33 = !DILocation(line: 5, column: 1, scope: !20)
!34 = !DILocation(line: 6, column: 1, scope: !20)
!35 = !DILocation(line: 7, column: 1, scope: !20)
