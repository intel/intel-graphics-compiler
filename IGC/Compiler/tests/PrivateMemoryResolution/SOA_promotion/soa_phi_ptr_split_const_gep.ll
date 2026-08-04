;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; splitPHIsOfAllocaPointers replays the whole pointer chain between the merge and
; the load on each predecessor, not just casts: collectLoadChains also follows
; constant-index GEPs (they dominate every predecessor, so they can be replayed),
; and cloneCastChainPHI recreates them, preserving `inbounds`.
;
; Two loads reach the merged pointer through such a GEP -- one inbounds, one not --
; so each predecessor gets both GEPs replayed. With the pointer merge gone, the alloca is
; left with a plain GEP/load chain and PrivateMemoryResolution SoA-transposes it.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-split-phis-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1,EnablePrivMemNewSOAForScalarArrays=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @test_load_phi_const_gep(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
;
; The stack predecessor is SoA-transposed: the byte offset of the replayed GEP is folded
; into the chunk arithmetic (constant + idx * 4, split by SOAPartitionBytes = 4),
; so no GEP of the merged pointer survives on this path.
;
; CHECK-LABEL: bb_stack:
; CHECK:       [[IDX:%.*]] = mul nsw i32 %{{.*}}, 4
; CHECK:       add i32 8, [[IDX]]
; CHECK:       lshr i32 %{{.*}}, 2
; CHECK:       load float
; CHECK:       add i32 4, [[IDX]]
; CHECK:       lshr i32 %{{.*}}, 2
; CHECK:       load float
;
; The non-alloca predecessor keeps the replayed GEPs verbatim, `inbounds` included.
;
; CHECK-LABEL: bb_extern:
; CHECK:       [[E0:%.*]] = getelementptr inbounds float, ptr addrspace(4) %ext_off, i64 2
; CHECK-NEXT:  load float, ptr addrspace(4) [[E0]]
; CHECK:       [[E1:%.*]] = getelementptr float, ptr addrspace(4) %ext_off, i64 1
; CHECK-NEXT:  load float, ptr addrspace(4) [[E1]]
;
; Both loads are merged at value level; no pointer PHI is left.
;
; CHECK-LABEL: join:
; CHECK:       phi float
; CHECK:       phi float
; CHECK-NOT:   phi ptr

define spir_kernel void @test_load_phi_const_gep(ptr addrspace(1) nocapture writeonly %d, ptr addrspace(4) %ext, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr nocapture readnone %privateBase) {
entry:
  %pb = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  br i1 %c, label %bb_stack, label %bb_extern

bb_stack:
  %g = getelementptr inbounds [33 x float], ptr %pb, i64 0, i64 %idx
  %g_as4 = addrspacecast ptr %g to ptr addrspace(4)
  br label %join

bb_extern:
  %ext_off = getelementptr inbounds i8, ptr addrspace(4) %ext, i64 4
  br label %join

join:
  %p = phi ptr addrspace(4) [ %g_as4, %bb_stack ], [ %ext_off, %bb_extern ]
  %pin = getelementptr inbounds float, ptr addrspace(4) %p, i64 2
  %v0 = load float, ptr addrspace(4) %pin, align 4
  %pnb = getelementptr float, ptr addrspace(4) %p, i64 1
  %v1 = load float, ptr addrspace(4) %pnb, align 4
  %v = fadd float %v0, %v1
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %arrayidx, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test_load_phi_const_gep}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{ptr @test_load_phi_const_gep, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !413, !414, !415, !416, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
