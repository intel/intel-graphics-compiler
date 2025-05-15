;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test is a modification of store_use_vector.ll and it's purpose is to make sure, that debug calls
; are neutral to LdStCombine pass optimizations. They should not be treated as fence-like calls.
;
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

;
; CHECK-LABEL: define spir_kernel void @f0
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.v8i32(ptr addrspace(1) %{{.*}}, <8 x i32> <i32 0, i32 1065353216, i32 1073741824, i32 117835012, i32 8, i32 202050057, i32 269422093, i32 336794129>, i64 32, i1 true)
; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Result = type { [3 x float], [4 x i8], i32, [12 x i8] }

; Function Attrs: convergent nounwind
define spir_kernel void @f0(ptr addrspace(1) %result, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
entry:
  %idxprom = zext i16 %localIdX to i64
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx1 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 0, i64 0
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) %arrayidx1, float 0.000000e+00, i64 32, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx5 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 0, i64 1
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) %arrayidx5, float 1.000000e+00, i64 4, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx9 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 0, i64 2
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) %arrayidx9, float 2.000000e+00, i64 8, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx12 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 1, i64 0
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx12, i8 4, i64 4, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx16 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 1, i64 1
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx16, i8 5, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx20 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 1, i64 2
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx20, i8 6, i64 2, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx24 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 1, i64 3
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx24, i8 7, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %test0 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 2
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %test0, i32 8, i64 16, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 0
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32, i8 9, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.1 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 1
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.1, i8 10, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.2 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 2
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.2, i8 11, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.3 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 3
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.3, i8 12, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.4 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 4
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.4, i8 13, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.5 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 5
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.5, i8 14, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.6 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 6
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.6, i8 15, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.7 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 7
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.7, i8 16, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.8 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 8
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.8, i8 17, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.9 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 9
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.9, i8 18, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.10 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 10
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.10, i8 19, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  %arrayidx32.11 = getelementptr inbounds %struct.Result, ptr addrspace(1) %result, i64 %idxprom, i32 3, i64 11
  call void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1) %arrayidx32.11, i8 20, i64 1, i1 true)
  call void @llvm.dbg.value(metadata i32 0, metadata !9, metadata !DIExpression()), !dbg !11
  ret void
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

declare void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1), float, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1.i8(ptr addrspace(1), i8, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1)

!igc.functions = !{!12}
!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "DI_call_betwen_stores.ll", directory: "/")
!2 = !{}
!3 = !{i32 13}
!4 = !{i32 11}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_dbginfo", linkageName: "test_dbginfo", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "testvar", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !{ptr @f0, !13}
!13 = !{!14}
!14 = !{!"function_type", i32 0}
