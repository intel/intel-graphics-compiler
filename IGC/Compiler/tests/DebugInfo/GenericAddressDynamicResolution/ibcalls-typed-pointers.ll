;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-generic-address-dynamic-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; GenericAddressDynamicResolution
; ------------------------------------------------
; This test checks that GenericAddressDynamicResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_kernel
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ACAST_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i8 addrspace(4)* [[ACAST_V]]
; CHECK-SAME: metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i8* [[IBP_V:%[A-z0-9.]*]], metadata [[IBP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IBP_LOC:![0-9]*]]
; CHECK-DAG: [[IBP_V]] = {{.*}} !dbg [[IBP_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i8 addrspace(1)* [[IBG_V:%[A-z0-9.]*]], metadata [[IBG_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IBG_LOC:![0-9]*]]
; CHECK-DAG: [[IBG_V]] = {{.*}} !dbg [[IBG_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i8 addrspace(3)* [[IBL_V:%[A-z0-9.]*]], metadata [[IBL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IBL_LOC:![0-9]*]]
; CHECK-DAG: [[IBL_V]] = {{.*}} !dbg [[IBL_LOC]]


define spir_kernel void @test_kernel(i32 addrspace(1)* %src) !dbg !6 {
  %1 = addrspacecast i32 addrspace(1)* %src to i8 addrspace(4)*, !dbg !14
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %1, metadata !9, metadata !DIExpression()), !dbg !14
  %2 = call i8* @__builtin_IB_to_private(i8 addrspace(4)* %1), !dbg !15
  call void @llvm.dbg.value(metadata i8* %2, metadata !11, metadata !DIExpression()), !dbg !15
  %3 = call i8 addrspace(1)* @__builtin_IB_to_global(i8 addrspace(4)* %1), !dbg !16
  call void @llvm.dbg.value(metadata i8 addrspace(1)* %3, metadata !12, metadata !DIExpression()), !dbg !16
  %4 = call i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* %1), !dbg !17
  call void @llvm.dbg.value(metadata i8 addrspace(3)* %4, metadata !13, metadata !DIExpression()), !dbg !17
  ret void, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ibcalls.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IBP_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[IBP_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IBG_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[IBG_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IBL_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[IBL_LOC]] =  !DILocation(line: 4, column: 1, scope: [[SCOPE]])

declare i8 addrspace(1)* @__builtin_IB_to_global(i8 addrspace(4)*)

declare i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)*)

declare i8* @__builtin_IB_to_private(i8 addrspace(4)*)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ibcalls.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocation(line: 1, column: 1, scope: !6)
!15 = !DILocation(line: 2, column: 1, scope: !6)
!16 = !DILocation(line: 3, column: 1, scope: !6)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
