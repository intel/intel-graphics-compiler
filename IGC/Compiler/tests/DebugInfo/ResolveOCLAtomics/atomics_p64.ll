;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-resolve-atomics -S < %s | FileCheck %s
; ------------------------------------------------
; ResolveOCLAtomics : atomics part with 64bit pointers
; ------------------------------------------------
; This test checks that ResolveOCLAtomics pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; ModuleID = 'ResolveOCLAtomics/atomics_p64.ll'
source_filename = "ResolveOCLAtomics/atomics_p64.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test_atomics(i32 %isrc, float %fsrc, i64 addrspace(1)* %gptr, i64 addrspace(3)* %lptr) !dbg !6 {
; Testcase 1:
; Check that call dbg info is preserved for cmpxchg32 atomic
; CHECK: [[ICMP_A:%[0-9]*]] = bitcast i64 addrspace(3)*
; CHECK: [[ICMP_V:%[0-9]*]] = call i32 @llvm.genx.GenISA.icmpxchgatomicraw{{.*}}(i32 addrspace(3)* [[ICMP_A]], {{.*}} !dbg [[ICMP_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[ICMP_V]],  metadata [[ICMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP_LOC]]
; CHECK-NEXT: [[DBG_DECLARE_CALL:dbg.declare\(metadata]] i32 addrspace(3)* [[ICMP_A]],  metadata [[ICMP_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP_LOC]]
  %1 = bitcast i64 addrspace(3)* %lptr to i32 addrspace(3)*, !dbg !22
  %2 = call i32 @__builtin_IB_atomic_cmpxchg_local_i32(i32 addrspace(3)* %1, i32 %isrc, i32 2), !dbg !23
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !23
  call void @llvm.dbg.declare(metadata i32 addrspace(3)* %1, metadata !9, metadata !DIExpression()), !dbg !23
; Testcase 2:
; Check that call dbg info is preserved for inc16 atomic
; CHECK: [[INC_A:%[0-9]*]] = bitcast i64 addrspace(1)*
; CHECK: [[INC_V:%[0-9]*]] = call i16 @llvm.genx.GenISA.intatomicrawA64{{.*}}(i16 addrspace(1)* [[INC_A]], {{.*}} !dbg [[INC_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i16 [[INC_V]],  metadata [[INC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INC_LOC]]
; CHECK-NEXT: [[DBG_DECLARE_CALL]] i16 addrspace(1)* [[INC_A]],  metadata [[INC_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INC_LOC]]
  %3 = bitcast i64 addrspace(1)* %gptr to i16 addrspace(1)*, !dbg !24
  %4 = call i16 @__builtin_IB_atomic_inc_global_i16(i16 addrspace(1)* %3), !dbg !25
  call void @llvm.dbg.value(metadata i16 %4, metadata !14, metadata !DIExpression()), !dbg !25
  call void @llvm.dbg.declare(metadata i16 addrspace(1)* %3, metadata !13, metadata !DIExpression()), !dbg !25
; Testcase 3:
; Check that call dbg info is preserved for add64 atomic
; CHECK: [[ADD_A:%[0-9]*]] = bitcast i16 addrspace(1)*
; CHECK: [[ADD_V:%[0-9]*]] = call i64 @llvm.genx.GenISA.intatomicrawA64{{.*}}(i64 addrspace(1)* [[ADD_A]], {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i64 [[ADD_V]],  metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK-NEXT: [[DBG_DECLARE_CALL]] i64 addrspace(1)* [[ADD_A]],  metadata [[ADD_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
  %5 = bitcast i16 addrspace(1)* %3 to i64 addrspace(1)*, !dbg !26
  %6 = call i64 @__builtin_IB_atomic_add_global_i64(i64 addrspace(1)* %5, i64 15), !dbg !27
  call void @llvm.dbg.value(metadata i64 %6, metadata !17, metadata !DIExpression()), !dbg !27
  call void @llvm.dbg.declare(metadata i64 addrspace(1)* %5, metadata !16, metadata !DIExpression()), !dbg !27
; Testcase 4:
; Check that call dbg info is preserved for fcmpxchg atomic
; CHECK: [[FCMP_A:%[0-9]*]] = bitcast i64 addrspace(1)*
; CHECK: [[FCMP_V:%[0-9]*]] = call float @llvm.genx.GenISA.fcmpxchgatomicrawA64{{.*}}(float addrspace(1)* [[FCMP_A]], {{.*}} !dbg [[FCMP_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] float [[FCMP_V]],  metadata [[FCMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_LOC]]
; CHECK-NEXT: [[DBG_DECLARE_CALL]] float addrspace(1)* [[FCMP_A]],  metadata [[FCMP_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_LOC]]
  %7 = bitcast i64 addrspace(1)* %gptr to float addrspace(1)*, !dbg !28
  %8 = call float @__builtin_IB_atomic_cmpxchg_global_f32(float addrspace(1)* %7, float %fsrc, float 3.000000e+00), !dbg !29
  call void @llvm.dbg.value(metadata float %8, metadata !19, metadata !DIExpression()), !dbg !29
  call void @llvm.dbg.declare(metadata float addrspace(1)* %7, metadata !18, metadata !DIExpression()), !dbg !29
; Testcase 5:
; Check that call dbg info is preserved for fmax atomic
; CHECK: [[FMAX_A:%[0-9]*]] = bitcast i64 addrspace(3)*
; CHECK: [[FMAX_V:%[0-9]*]] = call float @llvm.genx.GenISA.floatatomicraw{{.*}}(float addrspace(3)* [[FMAX_A]], {{.*}} !dbg [[FMAX_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] float [[FMAX_V]],  metadata [[FMAX_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FMAX_LOC]]
; CHECK-NEXT: [[DBG_DECLARE_CALL]] float addrspace(3)* [[FMAX_A]],  metadata [[FMAX_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FMAX_LOC]]
  %9 = bitcast i64 addrspace(3)* %lptr to float addrspace(3)*, !dbg !30
  %10 = call float @__builtin_IB_atomic_max_local_f32(float addrspace(3)* %9, float %fsrc), !dbg !31
  call void @llvm.dbg.value(metadata float %10, metadata !21, metadata !DIExpression()), !dbg !31
  call void @llvm.dbg.declare(metadata float addrspace(3)* %9, metadata !20, metadata !DIExpression()), !dbg !31
  ret void, !dbg !32
}

; Testcase 1 MD:
; CHECK-DAG: [[ICMP_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[ICMP_MD]] = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
; CHECK-DAG: [[ICMP_A_MD]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)

; Testcase 2 MD:
; CHECK-DAG: [[INC_LOC]] = !DILocation(line: 4
; CHECK-DAG: [[INC_MD]] = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
; CHECK-DAG: [[INC_A_MD]] = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)

; Testcase 3 MD:
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 6
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
; CHECK-DAG: [[ADD_A_MD]] = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)

; Testcase 4 MD:
; CHECK-DAG: [[FCMP_LOC]] = !DILocation(line: 8
; CHECK-DAG: [[FCMP_MD]] = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
; CHECK-DAG: [[FCMP_A_MD]] = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)

; Testcase 5 MD:
; CHECK-DAG: [[FMAX_LOC]] = !DILocation(line: 10
; CHECK-DAG: [[FMAX_MD]] = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !12)
; CHECK-DAG: [[FMAX_A_MD]] = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)

declare i16 @__builtin_IB_atomic_inc_global_i16(i16 addrspace(1)*)

declare i64 @__builtin_IB_atomic_add_global_i64(i64 addrspace(1)*, i64)

declare i32 @__builtin_IB_atomic_cmpxchg_local_i32(i32 addrspace(3)*, i32, i32)

declare float @__builtin_IB_atomic_cmpxchg_global_f32(float addrspace(1)*, float, float)

declare float @__builtin_IB_atomic_max_local_f32(float addrspace(3)*, float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ResolveOCLAtomics/atomics_p64.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 10}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_atomics", linkageName: "test_atomics", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !16, !17, !18, !19, !20, !21}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
!15 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)
!21 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !12)
!22 = !DILocation(line: 1, column: 1, scope: !6)
!23 = !DILocation(line: 2, column: 1, scope: !6)
!24 = !DILocation(line: 3, column: 1, scope: !6)
!25 = !DILocation(line: 4, column: 1, scope: !6)
!26 = !DILocation(line: 5, column: 1, scope: !6)
!27 = !DILocation(line: 6, column: 1, scope: !6)
!28 = !DILocation(line: 7, column: 1, scope: !6)
!29 = !DILocation(line: 8, column: 1, scope: !6)
!30 = !DILocation(line: 9, column: 1, scope: !6)
!31 = !DILocation(line: 10, column: 1, scope: !6)
!32 = !DILocation(line: 11, column: 1, scope: !6)
