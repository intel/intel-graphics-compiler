;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-legalization -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: load and store patterns
; ------------------------------------------------
; This test checks that Legalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

source_filename = "LoadStore.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test_ldst(i32 addrspace(1)* %load_a) !dbg !7 {
; Testcase 1:
; Load/Store i1 is propagated to i8
; Value for load is truncated
;
; CHECK: [[BITCAST_V:%[0-9]*]] = bitcast i1 addrspace(1)* {{.*}} to i8 addrspace(1)*, !dbg [[LOAD_LOC:![0-9]*]]
; CHECK-NEXT: [[LOAD8_V:%[0-9]*]] = load i8, i8 addrspace(1)* [[BITCAST_V]], align 4, !dbg [[LOAD_LOC]]
; CHECK-NEXT: [[LOAD1_V:%[0-9]*]] = trunc i8 %3 to i1, !dbg [[LOAD_LOC]]
; CHECK:  [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 %4, metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK-NEXT: [[ZEXT_V:%[0-9]*]] = zext i1 [[LOAD1_V]] to i8, !dbg [[STORE_LOC:![0-9]*]]
; CHECK-NEXT: [[BITCAST_V:%[0-9]*]] = bitcast i1 addrspace(1)* {{.*}} to i8 addrspace(1)*, !dbg [[STORE_LOC]]
; CHECK-NEXT: store i8 [[ZEXT_V]], i8 addrspace(1)* [[BITCAST_V]], align 4, !dbg [[STORE_LOC]]

  %1 = bitcast i32 addrspace(1)* %load_a to i1 addrspace(1)*, !dbg !18
  call void @llvm.dbg.value(metadata i1 addrspace(1)* %1, metadata !10, metadata !DIExpression()), !dbg !18
  %2 = load i1, i1 addrspace(1)* %1, align 4, !dbg !19
  call void @llvm.dbg.value(metadata i1 %2, metadata !12, metadata !DIExpression()), !dbg !19
  store i1 %2, i1 addrspace(1)* %1, align 4, !dbg !20

; Testcase 2:
; store with illegal int is replaced by store vector when possible
;
; MD Check:
; CHECK: [[ALLOC_V:%[0-9]*]] = alloca i96, align 4
; CHECK-NEXT: call void @llvm.dbg.declare(metadata i96* [[ALLOC_V]], metadata [[DECLARE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DECLARE_LOC:![0-9]*]]
; CHECK-NEXT: [[BITCASTV_V:%[0-9]*]] = bitcast i96 13 to <3 x i32>
; CHECK-NEXT: [[BITCASTA_V:%[0-9]*]] = bitcast i96* [[ALLOC_V]] to <3 x i32>*
; CHECK-NEXT:  store <3 x i32> [[BITCASTV_V]], <3 x i32>* [[BITCASTA_V]], align 4, !dbg [[STOREI_LOC:![0-9]*]]
  %3 = ptrtoint i32 addrspace(1)* %load_a to i32, !dbg !21
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !21
  %4 = alloca i96, align 4
  call void @llvm.dbg.declare(metadata i96* %4, metadata !16, metadata !DIExpression()), !dbg !22
  store i96 13, i96* %4, align 4, !dbg !23
; Testcase 3:
; store with const vector
;
; CHECK: [[INS1_V:%[0-9]*]] = insertelement <2 x i32> undef, i32 13, i32 0
; CHECK-NEXT: [[INS2_V:%[0-9]*]] = insertelement <2 x i32> [[INS1_V]], i32 12, i32 1
; CHECK-NEXT: store <2 x i32> [[INS2_V]], <2 x i32> addrspace(1)* {{.*}}, align 4, !dbg [[STOREV_LOC:![0-9]*]]
  %5 = bitcast i32 addrspace(1)* %load_a to <2 x i32> addrspace(1)*, !dbg !24
  call void @llvm.dbg.value(metadata <2 x i32> addrspace(1)* %5, metadata !17, metadata !DIExpression()), !dbg !24
  store <2 x i32> <i32 13, i32 12>, <2 x i32> addrspace(1)* %5, align 4, !dbg !25
  ret void, !dbg !26
}

; Testcase 1 MD:
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "2"
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 3
; Testcase 2 MD:
; CHECK-DAG: [[DECLARE_MD]] = !DILocalVariable(name: "4"
; CHECK-DAG: [[DECLARE_LOC]] = !DILocation(line: 5
; CHECK-DAG: [[STOREI_LOC]] = !DILocation(line: 6
; Testcase 3 MD:
; CHECK-DAG: [[STOREV_LOC]] = !DILocation(line: 8

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!4, !5}
!llvm.module.flags = !{!6}

!0 = !{void (i32 addrspace(1)*)* @test_ldst, !1}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "LoadStore.ll", directory: "/")
!4 = !{i32 9}
!5 = !{i32 5}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "test_ldst", linkageName: "test_ldst", scope: null, file: !3, line: 1, type: !8, scopeLine: 1, unit: !2, retainedNodes: !9)
!8 = !DISubroutineType(types: !1)
!9 = !{!10, !12, !14, !16, !17}
!10 = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
!11 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "3", scope: !7, file: !3, line: 4, type: !15)
!15 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "4", scope: !7, file: !3, line: 5, type: !11)
!17 = !DILocalVariable(name: "5", scope: !7, file: !3, line: 7, type: !11)
!18 = !DILocation(line: 1, column: 1, scope: !7)
!19 = !DILocation(line: 2, column: 1, scope: !7)
!20 = !DILocation(line: 3, column: 1, scope: !7)
!21 = !DILocation(line: 4, column: 1, scope: !7)
!22 = !DILocation(line: 5, column: 1, scope: !7)
!23 = !DILocation(line: 6, column: 1, scope: !7)
!24 = !DILocation(line: 7, column: 1, scope: !7)
!25 = !DILocation(line: 8, column: 1, scope: !7)
!26 = !DILocation(line: 9, column: 1, scope: !7)
