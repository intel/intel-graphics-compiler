;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-dp-to-fp-load-store -S < %s | FileCheck %s
; ------------------------------------------------
; HandleLoadStoreInstructions
; ------------------------------------------------
; This test checks that HandleLoadStoreInstructions pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_store
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ACAST1_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} addrspace(1)* [[ACAST1_V]]
; CHECK-SAME: metadata [[ACAST1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST1_LOC]]
;
; CHECK: [[ACAST2_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} addrspace(65536)* [[ACAST2_V]]
; CHECK-SAME: metadata [[ACAST2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST2_LOC]]
;
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store {{.*}} !dbg [[STORE2_LOC:![0-9]*]]


define spir_kernel void @test_store(double* %dst1, <2 x double>* %dstv) !dbg !6 {
  %1 = addrspacecast double* %dst1 to double addrspace(1)*, !dbg !12
  call void @llvm.dbg.value(metadata double addrspace(1)* %1, metadata !9, metadata !DIExpression()), !dbg !12
  %2 = addrspacecast <2 x double>* %dstv to <2 x double> addrspace(65536)*, !dbg !13
  call void @llvm.dbg.value(metadata <2 x double> addrspace(65536)* %2, metadata !11, metadata !DIExpression()), !dbg !13
  store double 1.000000e+00, double addrspace(1)* %1, !dbg !14
  store <2 x double> <double 1.000000e+00, double 0.000000e+00>, <2 x double> addrspace(65536)* %2, !dbg !15
  ret void, !dbg !16
}

;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "store.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_store", linkageName: "test_store", scope: null, file: [[FILE]], line: 1
;
; CHECK-DAG: [[ACAST1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ACAST2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ACAST2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "store.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_store", linkageName: "test_store", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocation(line: 1, column: 1, scope: !6)
!13 = !DILocation(line: 2, column: 1, scope: !6)
!14 = !DILocation(line: 3, column: 1, scope: !6)
!15 = !DILocation(line: 4, column: 1, scope: !6)
!16 = !DILocation(line: 5, column: 1, scope: !6)
