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
;
; This part simply adds MD to addrspacecast, thus check nothing changed except for that
; ------------------------------------------------

; CHECK: define spir_kernel void @test_kernel
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ACAST_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float addrspace(4)* [[ACAST_V]]
; CHECK-SAME: metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
;
; CHECK: [[PTRI1_V:%[0-9A-z]*]] = ptrtoint
; CHECK-SAME: !dbg [[PTRI1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[PTRI1_V]]
; CHECK-SAME: metadata [[PTRI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTRI1_LOC]]
;
; CHECK: [[GEP_V:%[0-9A-z]*]] = getelementptr
; CHECK-SAME: !dbg [[GEP_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float addrspace(4)* [[GEP_V]]
; CHECK-SAME: metadata [[GEP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP_LOC]]
;
; CHECK: [[PTRI2_V:%[0-9A-z]*]] = ptrtoint
; CHECK-SAME: !dbg [[PTRI2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[PTRI2_V]]
; CHECK-SAME: metadata [[PTRI2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTRI2_LOC]]
;
; CHECK: [[ADD_V:%[0-9A-z]*]] = add
; CHECK-SAME: !dbg [[ADD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD_V]]
; CHECK-SAME: metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]


define spir_kernel void @test_kernel(i32 addrspace(1)* %src) !dbg !6 {
  %1 = addrspacecast i32 addrspace(1)* %src to float addrspace(4)*, !dbg !16
  call void @llvm.dbg.value(metadata float addrspace(4)* %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = ptrtoint float addrspace(4)* %1 to i32, !dbg !17
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = getelementptr inbounds float, float addrspace(4)* %1, i16 13, !dbg !18
  call void @llvm.dbg.value(metadata float addrspace(4)* %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = ptrtoint float addrspace(4)* %3 to i32, !dbg !19
  call void @llvm.dbg.value(metadata i32 %4, metadata !14, metadata !DIExpression()), !dbg !19
  %5 = add i32 %2, %4, !dbg !20
  call void @llvm.dbg.value(metadata i32 %5, metadata !15, metadata !DIExpression()), !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "addrcast.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PTRI1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[PTRI1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEP_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[GEP_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PTRI2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[PTRI2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "addrcast.ll", directory: "/")
!2 = !{}
!3 = !{i32 6}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
