;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; SplitSelectsOfAllocaPointers replaces a load behind a pointer select with one
; GenISA_PredicatedLoad per operand, keeping the type of the original load. When
; that load reads a whole float3 field, LLVM's memcpy lowering has already turned
; it into a <3 x i32> access, so the predicated load is vector-typed too.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; sizeof(%SD) = 12 + 12 + 4 = 28 B, SOAPartitionBytes = 4.
%float3 = type { float, float, float }
%SD = type { %float3, %float3, i32 }

declare <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0.v3i32(ptr, i64, i1, <3 x i32>)
declare <3 x i16> @llvm.genx.GenISA.PredicatedLoad.v3i16.p0.v3i16(ptr, i64, i1, <3 x i16>)

; The per-lane stride is one chunk (4 B), not the whole 32-byte object, and the
; single <3 x i32> predicated load becomes three i32 predicated loads on the same
; predicate, reassembled with insertelement.
;
; CHECK-LABEL: @test_vector_predicated_load(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       mul i32 %{{.*}}, 4
; CHECK-NOT:   mul i32 %{{.*}}, 32
; CHECK:       [[C0:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr %{{.*}}, i64 4, i1 %pred, i32 %{{.*}})
; CHECK:       [[R0:%.*]] = insertelement <3 x i32> poison, i32 [[C0]], i32 0
; CHECK:       [[C1:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr %{{.*}}, i64 4, i1 %pred, i32 %{{.*}})
; CHECK:       [[R1:%.*]] = insertelement <3 x i32> [[R0]], i32 [[C1]], i32 1
; CHECK:       [[C2:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr %{{.*}}, i64 4, i1 %pred, i32 %{{.*}})
; CHECK:       [[R2:%.*]] = insertelement <3 x i32> [[R1]], i32 [[C2]], i32 2
; CHECK-NOT:   GenISA.PredicatedLoad.v3i32
; CHECK:       extractelement <3 x i32> [[R2]], i32 2
; CHECK:       ret void
define spir_kernel void @test_vector_predicated_load(ptr addrspace(1) nocapture %d, i1 %pred, <3 x i32> %merge,
                                                     <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %sd = alloca %SD, align 16
  %f1 = getelementptr inbounds %SD, ptr %sd, i64 0, i32 1
  store <3 x i32> zeroinitializer, ptr %f1, align 4
  %v = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0.v3i32(ptr %f1, i64 4, i1 %pred, <3 x i32> %merge)
  %e = extractelement <3 x i32> %v, i32 2
  store i32 %e, ptr addrspace(1) %d, align 4
  ret void
}

; Negative: <3 x i16> is 6 bytes -- neither a whole chunk nor a multiple of one --
; so it cannot be split chunk-wise and the alloca falls back to a contiguous
; per-lane AoS block (28 bytes rounded up to the alloca's 16-byte alignment).
;
; CHECK-LABEL: @test_vector_predicated_load_unaligned_negative(
; CHECK:       mul i32 %{{.*}}, 32
; CHECK:       GenISA.PredicatedLoad.v3i16
; CHECK:       ret void
define spir_kernel void @test_vector_predicated_load_unaligned_negative(ptr addrspace(1) nocapture %d, i1 %pred,
                                                                       <3 x i16> %merge, <8 x i32> %r0,
                                                                       <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %sd = alloca %SD, align 16
  %f1 = getelementptr inbounds %SD, ptr %sd, i64 0, i32 1
  store <3 x i32> zeroinitializer, ptr %f1, align 4
  %v = call <3 x i16> @llvm.genx.GenISA.PredicatedLoad.v3i16.p0.v3i16(ptr %f1, i64 2, i1 %pred, <3 x i16> %merge)
  %e = extractelement <3 x i16> %v, i32 2
  store i16 %e, ptr addrspace(1) %d, align 2
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !8, !9}
!4 = !{!"FuncMDMap[0]", ptr @test_vector_predicated_load}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", ptr @test_vector_predicated_load_unaligned_negative}
!9 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test_vector_predicated_load, !408}
!7 = !{ptr @test_vector_predicated_load_unaligned_negative, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
