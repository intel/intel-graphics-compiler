;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-generic-address-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; GenericAddressAnalysis
; ------------------------------------------------
; This test checks that GenericAddressAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; This pass updates Metadata, check that nothing debug related is affected
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_generica1{{.*}} !dbg [[SCOPE1:![0-9]*]]
;
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]

define spir_kernel void @test_generica1(i32 addrspace(4)* %a) !dbg !6 {
  %1 = load i32, i32 addrspace(4)* %a, !dbg !11
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !11
  store i32 %1, i32 addrspace(4)* %a, !dbg !12
  ret void, !dbg !13
}

; CHECK: @test_generica2{{.*}} !dbg [[SCOPE2:![0-9]*]]
;
; CHECK: [[ALLOCA_V:%[A-z0-9]*]] = alloca i32 addrspace(4)*{{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 addrspace(4)** [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: store {{.*}} !dbg [[STORE2_LOC:![0-9]*]]

define spir_kernel void @test_generica2(i32 addrspace(4)* %a) !dbg !14 {
  %1 = alloca i32 addrspace(4)*, align 4, !dbg !18
  call void @llvm.dbg.value(metadata i32 addrspace(4)** %1, metadata !16, metadata !DIExpression()), !dbg !18
  store i32 addrspace(4)* %a, i32 addrspace(4)** %1, !dbg !19
  ret void, !dbg !20
}

; CHECK: @test_generica3{{.*}} !dbg [[SCOPE3:![0-9]*]]
;
; CHECK: store {{.*}} !dbg [[STORE3_LOC:![0-9]*]]

define spir_kernel void @test_generica3(i32 addrspace(4)* %a) !dbg !21 {
  store i32 13, i32 addrspace(4)* %a, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenericAddressAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_generica1", linkageName: "test_generica1", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_generica2", linkageName: "test_generica2", scope: null, file: [[FILE]], line: 4
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE2]], file: [[FILE]], line: 4
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[SCOPE3]] = distinct !DISubprogram(name: "test_generica3", linkageName: "test_generica3", scope: null, file: [[FILE]], line: 7
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE3]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenericAddressAnalysis.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_generica1", linkageName: "test_generica1", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!14 = distinct !DISubprogram(name: "test_generica2", linkageName: "test_generica2", scope: null, file: !1, line: 4, type: !7, scopeLine: 4, unit: !0, retainedNodes: !15)
!15 = !{!16}
!16 = !DILocalVariable(name: "2", scope: !14, file: !1, line: 4, type: !17)
!17 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!18 = !DILocation(line: 4, column: 1, scope: !14)
!19 = !DILocation(line: 5, column: 1, scope: !14)
!20 = !DILocation(line: 6, column: 1, scope: !14)
!21 = distinct !DISubprogram(name: "test_generica3", linkageName: "test_generica3", scope: null, file: !1, line: 7, type: !7, scopeLine: 7, unit: !0, retainedNodes: !2)
!22 = !DILocation(line: 7, column: 1, scope: !21)
!23 = !DILocation(line: 8, column: 1, scope: !21)
