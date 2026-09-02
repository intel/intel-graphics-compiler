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
; Test that a heterogeneous struct array alloca is SoA-promoted even when one
; element is accessed via a direct struct*->T* bitcast (single-chunk access).
;
; The pattern arises in some workloads when ShaderData*->float* is used to read
; the first float field (P.x at byte offset 0) without going through a field GEP.
;
; The struct*->T* bitcast exception of SOALayoutChecker::visitBitCastInst accepts
; any T whose access spans whole SoA chunks, so both a scalar float and a vector
; whose *elements* are narrower than a chunk (<2 x i16>: 4 B = 1 x
; SOAPartitionBytes, as produced by LLVM's memcpy lowering of a pair of 16-bit
; fields) are covered here.
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=3,EnableAggressiveSOAPromotion=1  -S %s | FileCheck %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Heterogeneous struct: { float x, float y, i32 id, [4 x i8] padding }
; sizeof(%S) = 16 bytes.  All sub-fields are 4-byte aligned (or arrays whose
; total size is divisible by 4), so checkStruct passes and SOAPartitionBytes=4.
; The [4 x i8] field prevents flattening of the struct.
%S = type { float, float, i32, [4 x i8] }

; CHECK-LABEL: @test_float_bitcast
;;
;; Prolog: SoA uses simdLaneId x 4 (SOAPartitionBytes), NOT x 16 (struct stride)
;;
; CHECK:       [[laneId16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[laneId:%.*]]   = zext i16 [[laneId16]] to i32
; CHECK:       [[simdSize:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
;
; CHECK:       mul i32 [[laneId]], 4
; CHECK-NOT:   mul i32 [[laneId]], 16
;
;; The struct*->float* bitcast store hits field 0 at byte offset 0 => SoA chunk 0:
;; the element byte offset (idx * sizeof(%S)) is fed straight into the /4 with
;; no constant addend.
;;
; CHECK:       [[boff0:%.*]] = mul nsw i32 %{{.*}}, 16
; CHECK-NEXT:  lshr i32 [[boff0]], 2
; CHECK:       store float 1.000000e+00
;;
;; The regular field-1 GEP load is at byte offset 4 => SoA chunk 1: the offset-4
;; addend is folded in (add ..., 4) before dividing by SOAPartitionBytes.
;;
; CHECK:       [[boff1:%.*]] = add nsw i32 %{{.*}}, 4
; CHECK-NEXT:  lshr i32 [[boff1]], 2
; CHECK:       load float
;
; CHECK:       ret void

define spir_kernel void @test_float_bitcast(i32 addrspace(1)* nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %phsc = extractelement <8 x i32> %payloadHeader, i64 0
  %elsc = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0s18 = extractelement <8 x i32> %r0, i64 1
  %tmp0 = mul i32 %elsc, %r0s18
  %lx = zext i16 %localIdX to i32
  %tmp1 = add i32 %tmp0, %lx
  %ix = add i32 %tmp1, %phsc
  %idx = zext i32 %ix to i64
  %arr = alloca [64 x %S], align 4
  %arr8 = bitcast [64 x %S]* %arr to i8*
  call void @llvm.lifetime.start.p0i8(i64 1024, i8* nonnull %arr8)

  ; Access element %idx as %S* then bitcast to float* — single-chunk (4B = 1 × SOAPartitionBytes).
  %ep = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx
  %fp = bitcast %S* %ep to float*
  store float 1.0, float* %fp, align 4

  ; Regular field-1 access via GEP — field 1 is at byte offset 4 (chunk 1)
  %f1p = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx, i32 1
  %fval = load float, float* %f1p, align 4

  call void @llvm.lifetime.end.p0i8(i64 1024, i8* nonnull %arr8)
  %fval_bits = bitcast float %fval to i32
  %out = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 %fval_bits, i32 addrspace(1)* %out, align 4
  ret void
}

; CHECK-LABEL: @test_vec2i16_bitcast
;;
;; Same SoA prolog: simdLaneId x 4, not x 16.
;;
; CHECK:       [[laneId16v:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[laneIdv:%.*]]   = zext i16 [[laneId16v]] to i32
; CHECK:       mul i32 [[laneIdv]], 4
; CHECK-NOT:   mul i32 [[laneIdv]], 16
;;
;; The <2 x i16> store covers exactly chunk 0 of the element, so it lowers to a single
;; transposed store of the whole chunk (no per-i16 scatter).
;;
; CHECK:       [[boff0v:%.*]] = mul nsw i32 %{{.*}}, 16
; CHECK-NEXT:  lshr i32 [[boff0v]], 2
; CHECK:       store <2 x i16> <i16 1, i16 2>
;;
;; Field 1 at byte offset 4 => chunk 1, as in the float-bitcast kernel.
;;
; CHECK:       [[boff1v:%.*]] = add nsw i32 %{{.*}}, 4
; CHECK-NEXT:  lshr i32 [[boff1v]], 2
; CHECK:       load float
;
; CHECK:       ret void

define spir_kernel void @test_vec2i16_bitcast(i32 addrspace(1)* nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %phsc = extractelement <8 x i32> %payloadHeader, i64 0
  %elsc = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0s18 = extractelement <8 x i32> %r0, i64 1
  %tmp0 = mul i32 %elsc, %r0s18
  %lx = zext i16 %localIdX to i32
  %tmp1 = add i32 %tmp0, %lx
  %ix = add i32 %tmp1, %phsc
  %idx = zext i32 %ix to i64
  %arr = alloca [64 x %S], align 4
  %arr8 = bitcast [64 x %S]* %arr to i8*
  call void @llvm.lifetime.start.p0i8(i64 1024, i8* nonnull %arr8)

  ; Access element %idx as %S* then bitcast to <2 x i16>* — one chunk, sub-chunk elements.
  %ep = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx
  %vp = bitcast %S* %ep to <2 x i16>*
  store <2 x i16> <i16 1, i16 2>, <2 x i16>* %vp, align 4

  ; Regular field-1 access via GEP — field 1 is at byte offset 4 (chunk 1)
  %f1p = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx, i32 1
  %fval = load float, float* %f1p, align 4

  call void @llvm.lifetime.end.p0i8(i64 1024, i8* nonnull %arr8)
  %fval_bits = bitcast float %fval to i32
  %out = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 %fval_bits, i32 addrspace(1)* %out, align 4
  ret void
}

declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture)

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !8, !9}
!4 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_float_bitcast}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_vec2i16_bitcast}
!9 = !{!"FuncMDValue[1]", !2}
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_float_bitcast, !408}
!7 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_vec2i16_bitcast, !408}
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
