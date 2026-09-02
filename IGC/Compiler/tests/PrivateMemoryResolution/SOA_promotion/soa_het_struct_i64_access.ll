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
; Test that a heterogeneous struct alloca ([64 x {float, float, i32}]) is
; promoted to SoA even when two adjacent 4B float fields are accessed as a
; single i64 (LLVM's memcpy-expansion pattern: bitcast float* → i64*).
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1  -S %s | FileCheck %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Heterogeneous struct with two adjacent floats + i32.
; All leaf scalars are 4 bytes, so SoA-eligible.
; LLVM's memcpy lowering may access both floats as a single i64.
%S = type { float, float, i32 }

; CHECK-LABEL: @test
;;
;; Prolog: SoA lane offset must use SOAPartitionBytes=4, NOT struct stride 12
;;
; CHECK:       [[laneId16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[laneId:%.*]]   = zext i16 [[laneId16]] to i32
; CHECK:       [[simdSize:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       mul i32 [[laneId]], 4
; CHECK-NOT:   mul i32 [[laneId]], 12
;;
;; i64 store is split into two separate i32 stores to different SoA chunks.
;; The address of each chunk is computed with simdSize, then the constant i64
;; is recast as <2 x i32> and stored element-by-element.
;;
; CHECK:       mul i32 [[simdSize]]
; CHECK:       store i32 extractelement (<2 x i32>
; CHECK:       mul i32 [[simdSize]]
; CHECK:       store i32 extractelement (<2 x i32>
;;
;; i64 load is split into two separate i32 loads, reassembled via insertelement
;; into <2 x i32>, then bitcast back to i64.
;;
; CHECK:       mul i32 [[simdSize]]
; CHECK:       load i32
; CHECK:       insertelement <2 x i32>
; CHECK:       mul i32 [[simdSize]]
; CHECK:       load i32
; CHECK:       insertelement <2 x i32>
; CHECK:       bitcast <2 x i32> {{.*}} to i64

define spir_kernel void @test(i32 addrspace(1)* nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
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
  call void @llvm.lifetime.start.p0i8(i64 768, i8* nonnull %arr8)
  ; Store two adjacent floats as a single i64 (LLVM memcpy-expansion pattern)
  %sf0 = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx, i32 0
  %scast = bitcast float* %sf0 to i64*
  store i64 4614256657445502976, i64* %scast, align 4
  ; Load two adjacent floats as a single i64
  %lf0 = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx, i32 0
  %lcast = bitcast float* %lf0 to i64*
  %val64 = load i64, i64* %lcast, align 4
  ; Regular i32 read of the third field
  %ip = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx, i32 2
  %id = load i32, i32* %ip, align 4
  call void @llvm.lifetime.end.p0i8(i64 768, i8* nonnull %arr8)
  %v64lo = trunc i64 %val64 to i32
  %vout = add i32 %v64lo, %id
  %out = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 %vout, i32 addrspace(1)* %out, align 4
  ret void
}

declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture)

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test}
!5 = !{!"FuncMDValue[1]", !2}
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test, !408}
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
