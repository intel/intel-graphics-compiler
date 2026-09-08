;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; Test that a heterogeneous struct alloca is SoA-promoted even when an inner
; sub-array is indexed via a *dynamic* i8-byte GEP whose offset is provably a
; multiple of SOAPartitionBytes.
;
; InstCombine canonicalizes
;     getelementptr %ShaderData, %sd, 0, <member>, %idx, <field>
; into
;     %base = getelementptr %ShaderData, %sd, 0, <member>, 0, <field>
;     %bc   = bitcast <leaf>* %base to i8*
;     %mul  = mul nsw i64 %idx, sizeof(<element>)
;     %dyn  = getelementptr i8, i8* %bc, i64 %mul
;
; If each element size is a multiple of the partition size, then any runtime computed element offset will also
; be aligned to that partition size. The compiler can recognize this because the offsets lower log2(SOAPartitionBytes)
; bits are guaranteed to be zero.
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:         --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Heterogeneous outer struct holds a leading i32 plus a [64 x %Elem] array.
; %Elem is itself heterogeneous so outer is not flattened to a vector.
;
; sizeof(%Elem) = 16 bytes (multiple of SOAPartitionBytes=4) — so a dynamic
; byte stride of 16 has 4 trailing zero bits, satisfying KnownBits >= 2.
%Elem = type { float, i32, float, i32 }
%S    = type { i32, [64 x %Elem] }

; CHECK-LABEL: @test_dynamic_i8_gep_partition_aligned
;;
;; SoA promotion succeeded: per-lane chunks of SOAPartitionBytes (=4),
;; not a contiguous per-lane block of sizeof(%S) = 1028.
;;
; CHECK:       call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       mul i32 %{{.*}}, 4
; CHECK-NOT:   mul i32 %{{.*}}, 1028
;;
;; Dynamic-stride store lowering. The runtime byte offset is truncated to i32
;; and added to the constant base offset (12 = 4-byte leading i32 + field-2
;; offset 8). Dividing by SOAPartitionBytes (lshr 2) gives the chunk index,
;; which is scaled back by SOAPartitionBytes (*4) and by the per-lane simdSize
;; stride to form the final per-lane address.
;;
; CHECK:       [[TRUNC:%.*]] = trunc i64 %{{.*}} to i32
; CHECK:       add i32 {{.*}}, 12
; CHECK:       [[CHUNK:%.*]] = lshr i32 %{{.*}}, 2
; CHECK:       mul i32 [[CHUNK]], 4
; CHECK:       mul i32 [[SIMD:%.*]], %{{.*}}
; CHECK:       store float 3.500000e+00
; CHECK:       ret void

define spir_kernel void @test_dynamic_i8_gep_partition_aligned(i32 addrspace(1)* nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %phsc  = extractelement <8 x i32> %payloadHeader, i64 0
  %elsc  = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0s18 = extractelement <8 x i32> %r0, i64 1
  %tmp0  = mul i32 %elsc, %r0s18
  %lx    = zext i16 %localIdX to i32
  %tmp1  = add i32 %tmp0, %lx
  %ix    = add i32 %tmp1, %phsc

  %arr   = alloca %S, align 4
  %arr8  = bitcast %S* %arr to i8*
  call void @llvm.lifetime.start.p0i8(i64 1028, i8* nonnull %arr8)

  ; Constant-offset typed GEP into the inner array's element 0, field 2 (a float).
  ; Yields a float* whose byte offset within the alloca is partition-aligned.
  %base = getelementptr inbounds %S, %S* %arr, i64 0, i32 1, i64 0, i32 2
  %bc   = bitcast float* %base to i8*

  ; Dynamic byte offset: %ix * 16 (= sizeof(%Elem)).  KnownBits trailing
  ; zeros = 4 ≥ log2(SOAPartitionBytes=4) = 2, so MismatchDetected accepts it.
  %ix64 = zext i32 %ix to i64
  %mul  = mul nsw i64 %ix64, 16
  %dyn8 = getelementptr i8, i8* %bc, i64 %mul
  %dynp = bitcast i8* %dyn8 to float*
  store float 3.5, float* %dynp, align 4

  ; A second, constant-offset typed GEP into the same alloca. This forces the
  ; SoA lowering to handle both access kinds on one promoted object: the runtime
  ; (byte-GEP) path above and this compile time constant path, which must resolve
  ; to the same per-lane layout.
  %fp0 = getelementptr inbounds %S, %S* %arr, i64 0, i32 1, i64 0, i32 0
  %fv  = load float, float* %fp0, align 4

  call void @llvm.lifetime.end.p0i8(i64 1028, i8* nonnull %arr8)

  %fb  = bitcast float %fv to i32
  %out = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %ix64
  store i32 %fb, i32 addrspace(1)* %out, align 4
  ret void
}

; CHECK-LABEL: @test_dynamic_i8_gep_misaligned_negative
;;
;; Negative test: the dynamic addend is %ix * 3 — KnownBits trailing zeros = 0,
;; so MismatchDetected must keep rejecting it. The alloca falls through to AoS
;; lowering: per-lane stride = sizeof(%S) = 1028.
;;
; CHECK:       mul i32 %{{.*}}, 1028
; CHECK:       ret void

define spir_kernel void @test_dynamic_i8_gep_misaligned_negative(i32 addrspace(1)* nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %phsc  = extractelement <8 x i32> %payloadHeader, i64 0
  %elsc  = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0s18 = extractelement <8 x i32> %r0, i64 1
  %tmp0  = mul i32 %elsc, %r0s18
  %lx    = zext i16 %localIdX to i32
  %tmp1  = add i32 %tmp0, %lx
  %ix    = add i32 %tmp1, %phsc

  %arr   = alloca %S, align 4
  %arr8  = bitcast %S* %arr to i8*
  call void @llvm.lifetime.start.p0i8(i64 1028, i8* nonnull %arr8)

  %base = getelementptr inbounds %S, %S* %arr, i64 0, i32 1, i64 0, i32 2
  %bc   = bitcast float* %base to i8*

  ; Misaligned dynamic stride: 3 is not a multiple of SOAPartitionBytes=4.
  %ix64 = zext i32 %ix to i64
  %mul  = mul nsw i64 %ix64, 3
  %dyn8 = getelementptr i8, i8* %bc, i64 %mul
  %dynp = bitcast i8* %dyn8 to i8*
  store i8 7, i8* %dynp, align 1

  call void @llvm.lifetime.end.p0i8(i64 1028, i8* nonnull %arr8)
  store i32 0, i32 addrspace(1)* %d, align 4
  ret void
}

declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture)

!IGCMetadata = !{!0}
!igc.functions = !{!6, !20}

!0  = !{!"ModuleMD", !1, !3}
!1  = !{!"compOpt", !2}
!2  = !{!"UseScratchSpacePrivateMemory", i1 true}
!3  = !{!"FuncMD", !4, !5, !14, !15}
!4  = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_dynamic_i8_gep_partition_aligned}
!5  = !{!"FuncMDValue[1]", !2}
!14 = !{!"FuncMDMap[2]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_dynamic_i8_gep_misaligned_negative}
!15 = !{!"FuncMDValue[2]", !2}

!6  = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_dynamic_i8_gep_partition_aligned, !408}
!20 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_dynamic_i8_gep_misaligned_negative, !408}

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
