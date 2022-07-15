;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-fix-resource-ptr -S < %s | FileCheck %s
; ------------------------------------------------
; FixResourcePtr
; ------------------------------------------------
; This test checks that FixResourcePtr pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @foo
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value{{.*}} addrspace
; CHECK-SAME: metadata [[BUFP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BUFP_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value{{.*}} addrspace
; CHECK-SAME: metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC:![0-9]*]]
;
; CHECK: [[LOADV_V:%[0-9]*]] = {{.*}}<2 x i16>
; CHECK-SAME: !dbg [[LOADV_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value{{.*}} [[LOADV_V]]
; CHECK-SAME: metadata [[LOADV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOADV_LOC]]
;
; CHECK: {{.*}}<2 x i16> {{.*}} [[STOREV_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value{{.*}}addrspace
; CHECK-SAME: !dbg [[GEP_LOC:![0-9]*]]
;
; CHECK: [[LOAD_V:%[0-9]*]] = {{.*}}i32
; CHECK-SAME: !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value{{.*}} [[LOAD_V]]
; CHECK-SAME: metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
;
; CHECK: {{.*}}i32 {{.*}} [[STORE_LOC:![0-9]*]]

define spir_kernel void @foo(i32 %src) !dbg !6 {
  %1 = call i32 addrspace(131072)* @llvm.genx.GenISA.GetBufferPtr.p131072i32(i32 13, i32 1), !dbg !16
  call void @llvm.dbg.value(metadata i32 addrspace(131072)* %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = bitcast i32 addrspace(131072)* %1 to <2 x i16> addrspace(131072)*, !dbg !17
  call void @llvm.dbg.value(metadata <2 x i16> addrspace(131072)* %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = load <2 x i16>, <2 x i16> addrspace(131072)* %2, !dbg !18
  call void @llvm.dbg.value(metadata <2 x i16> %3, metadata !12, metadata !DIExpression()), !dbg !18
  store <2 x i16> %3, <2 x i16> addrspace(131072)* %2, !dbg !19
  %4 = getelementptr inbounds i32, i32 addrspace(131072)* %1, i64 4, !dbg !20
  call void @llvm.dbg.value(metadata i32 addrspace(131072)* %4, metadata !14, metadata !DIExpression()), !dbg !20
  %5 = load i32, i32 addrspace(131072)* %4, !dbg !21
  call void @llvm.dbg.value(metadata i32 %5, metadata !15, metadata !DIExpression()), !dbg !21
  store i32 %src, i32 addrspace(131072)* %4, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "direct.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: [[FILE]], line: 1
;
; CHECK-DAG: [[BUFP_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[BUFP_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[LOADV_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOADV_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[STOREV_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[GEP_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

declare i32 addrspace(131072)* @llvm.genx.GenISA.GetBufferPtr.p131072i32(i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "direct.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !13)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
