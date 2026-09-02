;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; An access wider than the SOA partition is lowered by the new-algo transpose as
; one access per chunk. When the access type is already a fixed vector whose
; element size equals the partition size, TransposePrivMem::getMultiChunkVecTy
; hands the vector back unchanged: the chunks are its own elements, so no
; reinterpreting bitcast is needed (unlike the i64 case, which becomes
; <2 x i32>).
;
; Here the alloca is an array of a heterogeneous struct { float, i32 } -- partition
; 4 bytes -- and both fields are accessed at once with a <2 x float>. The
; multi-partition-vector veto only applies to non-struct base types, so the new
; algo is used.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=3,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%st = type { float, i32 }

; The variable-index store writes the two elements to two chunks whose per-lane
; addresses are computed separately (byte offset -> chunk number via lshr 2).
;
; CHECK-LABEL: @test_vec_chunk_access(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       [[OFF:%.*]] = mul nsw i32 %{{.*}}, 8
; CHECK:       add i32 [[OFF]], 0
; CHECK:       lshr i32 %{{.*}}, 2
; CHECK:       store float 1.000000e+00, ptr %{{.*}}, align 4
; CHECK:       add i32 [[OFF]], 4
; CHECK:       lshr i32 %{{.*}}, 2
; CHECK:       store float 2.000000e+00, ptr %{{.*}}, align 4
;
; The load reads the two chunks and rebuilds the vector with insertelement --
; the element type is already chunk-wide, so nothing is bitcast.
;
; CHECK:       mul i32 [[SIMDSIZE]], 24
; CHECK:       [[E0:%.*]] = load float, ptr %{{.*}}, align 4
; CHECK:       [[V0:%.*]] = insertelement <2 x float> poison, float [[E0]], i32 0
; CHECK:       mul i32 [[SIMDSIZE]], 28
; CHECK:       [[E1:%.*]] = load float, ptr %{{.*}}, align 4
; CHECK:       [[V1:%.*]] = insertelement <2 x float> [[V0]], float [[E1]], i32 1
; CHECK:       extractelement <2 x float> [[V1]], i32 1
; CHECK-NOT:   bitcast
define spir_kernel void @test_vec_chunk_access(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %pb = alloca [8 x %st], align 8
  %idx = zext i32 %ix to i64
  %g0 = getelementptr inbounds [8 x %st], ptr %pb, i64 0, i64 %idx, i32 0
  store <2 x float> <float 1.000000e+00, float 2.000000e+00>, ptr %g0, align 8
  %g1 = getelementptr inbounds [8 x %st], ptr %pb, i64 0, i64 3, i32 0
  %v = load <2 x float>, ptr %g1, align 8
  %e = extractelement <2 x float> %v, i32 1
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %e, ptr addrspace(1) %arrayidx, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test_vec_chunk_access}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{ptr @test_vec_chunk_access, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
