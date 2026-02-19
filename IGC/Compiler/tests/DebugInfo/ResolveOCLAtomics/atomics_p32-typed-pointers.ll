;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-resolve-atomics -S < %s | FileCheck %s
; ------------------------------------------------
; ResolveOCLAtomics : atomics part with 32bit pointers
; ------------------------------------------------
; This test checks that ResolveOCLAtomics pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'ResolveOCLAtomics/atomics.ll'
source_filename = "ResolveOCLAtomics/atomics.ll"
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test_atomics(i32* %dst, i32 %isrc, float %fsrc) !dbg !6 {

; Testcase 1:
; Check that call dbg info is preserved for icmpxchg32 atomic
; CHECK: [[ICMP_V:%[0-9]*]] = call i32 @llvm.genx.GenISA.icmpxchgatomicraw.i32.p0i32.i32(i32* %dst, {{.*}} !dbg [[ICMP_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[ICMP_V]],  metadata [[ICMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP_LOC]]
  %1 = call i32 @__builtin_IB_atomic_cmpxchg_global_i32(i32* %dst, i32 %isrc, i32 0), !dbg !21
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !21
; Testcase 2:
; Check that call dbg info is preserved for inc64 atomic
; CHECK: [[INC_A:%[0-9]*]] = alloca i64
; CHECK: [[INC_V:%[0-9]*]] = call i64 @llvm.genx.GenISA.intatomicraw.i64.p0i64.i32(i64* [[INC_A]], {{.*}} !dbg [[INC_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_DECLARE_CALL:dbg.declare\(metadata]] i64* [[INC_A]],  metadata [[INC_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INC_LOC]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i64 [[INC_V]],  metadata [[INC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INC_LOC]]
  %2 = alloca i64, align 4, !dbg !22
  %3 = call i64 @__builtin_IB_atomic_inc_global_i64(i64* %2), !dbg !23
  call void @llvm.dbg.declare(metadata i64* %2, metadata !11, metadata !DIExpression()), !dbg !23
  call void @llvm.dbg.value(metadata i64 %3, metadata !13, metadata !DIExpression()), !dbg !23
; Testcase 3:
; Check that call dbg info is preserved for or16 atomic
; CHECK: [[OR_A:%[0-9]*]] = alloca i16
; CHECK: [[OR_V:%[0-9]*]] = call i16 @llvm.genx.GenISA.intatomicraw.i16.p0i16.i32(i16* [[OR_A]], {{.*}} !dbg [[OR_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_DECLARE_CALL]] i16* [[OR_A]],  metadata [[OR_A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[OR_LOC]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i16 [[OR_V]],  metadata [[OR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[OR_LOC]]
  %4 = alloca i16, align 4, !dbg !24
  %5 = trunc i32 %isrc to i16, !dbg !25
  call void @llvm.dbg.value(metadata i16 %5, metadata !15, metadata !DIExpression()), !dbg !25
  %6 = call i16 @__builtin_IB_atomic_or_global_i16(i16* %4, i16 %5), !dbg !26
  call void @llvm.dbg.declare(metadata i16* %4, metadata !14, metadata !DIExpression()), !dbg !26
  call void @llvm.dbg.value(metadata i16 %6, metadata !17, metadata !DIExpression()), !dbg !26
; Testcase 4:
; Check that call dbg info is preserved for fcmpxchg32 atomic
; CHECK: [[FCMP_V:%[0-9]*]] = call float @llvm.genx.GenISA.fcmpxchgatomicraw.f32.p0f32.i32({{.*}} !dbg [[FCMP_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] float [[FCMP_V]],  metadata [[FCMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_LOC]]
  %7 = bitcast i32* %dst to float*, !dbg !27
  call void @llvm.dbg.value(metadata float* %7, metadata !18, metadata !DIExpression()), !dbg !27
  %8 = call float @__builtin_IB_atomic_cmpxchg_global_f32(float* %7, float %fsrc, float 0.000000e+00), !dbg !28
  call void @llvm.dbg.value(metadata float %8, metadata !19, metadata !DIExpression()), !dbg !28
; Testcase 5:
; Check that call dbg info is preserved for fmin32 atomic
; CHECK: [[FMIN_V:%[0-9]*]] = call float @llvm.genx.GenISA.floatatomicraw.f32.p0f32.i32({{.*}} !dbg [[FMIN_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] float [[FMIN_V]],  metadata [[FMIN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FMIN_LOC]]
  %9 = call float @__builtin_IB_atomic_min_global_f32(float* %7, float %fsrc), !dbg !29
  call void @llvm.dbg.value(metadata float %9, metadata !20, metadata !DIExpression()), !dbg !29
  ret void, !dbg !30
}

; Testcase 1 MD:
; CHECK-DAG: [[ICMP_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[ICMP_MD]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)

; Testcase 2 MD:
; CHECK-DAG: [[INC_LOC]] = !DILocation(line: 3
; CHECK-DAG: [[INC_MD]] = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
; CHECK-DAG: [[INC_A_MD]] = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)

; Testcase 3 MD:
; CHECK-DAG: [[OR_LOC]] = !DILocation(line: 6
; CHECK-DAG: [[OR_MD]] = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !16)
; CHECK-DAG: [[OR_A_MD]] = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)

; Testcase 4 MD:
; CHECK-DAG: [[FCMP_LOC]] = !DILocation(line: 8
; CHECK-DAG: [[FCMP_MD]] = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)

; Testcase 5 MD:
; CHECK-DAG: [[FMIN_LOC]] = !DILocation(line: 9
; CHECK-DAG: [[FMIN_MD]] = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)

declare i64 @__builtin_IB_atomic_inc_global_i64(i64*)

declare i16 @__builtin_IB_atomic_or_global_i16(i16*, i16)

declare i32 @__builtin_IB_atomic_cmpxchg_global_i32(i32*, i32, i32)

declare float @__builtin_IB_atomic_cmpxchg_global_f32(float*, float, float)

declare float @__builtin_IB_atomic_min_global_f32(float*, float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ResolveOCLAtomics/atomics.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_atomics", linkageName: "test_atomics", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !17, !18, !19, !20}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !16)
!16 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !16)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !12)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)
!21 = !DILocation(line: 1, column: 1, scope: !6)
!22 = !DILocation(line: 2, column: 1, scope: !6)
!23 = !DILocation(line: 3, column: 1, scope: !6)
!24 = !DILocation(line: 4, column: 1, scope: !6)
!25 = !DILocation(line: 5, column: 1, scope: !6)
!26 = !DILocation(line: 6, column: 1, scope: !6)
!27 = !DILocation(line: 7, column: 1, scope: !6)
!28 = !DILocation(line: 8, column: 1, scope: !6)
!29 = !DILocation(line: 9, column: 1, scope: !6)
!30 = !DILocation(line: 10, column: 1, scope: !6)
