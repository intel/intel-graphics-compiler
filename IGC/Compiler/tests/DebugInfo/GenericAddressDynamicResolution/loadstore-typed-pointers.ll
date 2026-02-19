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
; CHECK: @llvm.dbg.value(metadata float addrspace(4)* [[ACAST_V]]
; CHECK-SAME: metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata float [[LOAD_V:%[A-z0-9.]*]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD_V]] = {{.*}} !dbg [[LOAD_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata float [[FADD_V:%[A-z0-9.]*]], metadata [[FADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FADD_LOC:![0-9]*]]
; CHECK-DAG: [[FADD_V]] = {{.*}} !dbg [[FADD_LOC]]
;
; CHECK: store float [[FADD_V]]
; CHECK-SAME: !dbg [[STORE_LOC:![0-9]*]]

define spir_kernel void @test_kernel(i32 addrspace(1)* %src) !dbg !6 {
  %1 = addrspacecast i32 addrspace(1)* %src to float addrspace(4)*, !dbg !14
  call void @llvm.dbg.value(metadata float addrspace(4)* %1, metadata !9, metadata !DIExpression()), !dbg !14
  %2 = load float, float addrspace(4)* %1, align 8, !dbg !15
  call void @llvm.dbg.value(metadata float %2, metadata !11, metadata !DIExpression()), !dbg !15
  %3 = fadd float %2, 1.000000e+00, !dbg !16
  call void @llvm.dbg.value(metadata float %3, metadata !13, metadata !DIExpression()), !dbg !16
  store float %3, float addrspace(4)* %1, align 4, !dbg !17
  ret void, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "loadstore.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FADD_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[FADD_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] =  !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "loadstore.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocation(line: 1, column: 1, scope: !6)
!15 = !DILocation(line: 2, column: 1, scope: !6)
!16 = !DILocation(line: 3, column: 1, scope: !6)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
