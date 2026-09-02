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
; Test that a heterogeneous struct (Vec3 sub-struct + float + i32) inside an
; array alloca is promoted to SoA when EnablePrivMemNewSOATranspose=1 on PTL.
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1  -S %s | FileCheck %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Heterogeneous struct: Vec3 (3xfloat sub-struct) + float + i32
; sizeof(Particle) = 20 bytes, all leaf scalars are 4 bytes → SoA-eligible
%struct.Vec3     = type { float, float, float }
%struct.Particle = type { %struct.Vec3, float, i32 }

; CHECK-LABEL: @test
;;
;; Prolog: simdLaneId and simdSize must be present (SoA path uses them)
;;
; CHECK:       [[simdLaneId16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[simdLaneId:%.*]]   = zext i16 [[simdLaneId16]] to i32
; CHECK:       [[simdSize:%.*]]     = call i32 @llvm.genx.GenISA.simdSize()
;;
;; SOA base: laneId * SOAPartitionBytes(4), NOT laneId * 1280 (full alloca)
;;
; CHECK:       [[T0:%.*]] = mul i32 [[simdLaneId]], 4
; CHECK-NOT:   mul i32 [[simdLaneId]], 1280
;;
;; Each field access uses simdSize * (field_chunk_offset) stride — SoA interleave
;;
; CHECK:       [[T1:%.*]] = mul i32 [[simdSize]], {{.*}}
; CHECK:       [[T2:%.*]] = mul i32 [[simdSize]], {{.*}}
; CHECK:       [[T3:%.*]] = mul i32 [[simdSize]], {{.*}}
; CHECK:       [[T4:%.*]] = mul i32 [[simdSize]], {{.*}}
;
; CHECK:   ret void

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
  %arr = alloca [64 x %struct.Particle], align 4
  %arr8 = bitcast [64 x %struct.Particle]* %arr to i8*
  call void @llvm.lifetime.start.p0i8(i64 1280, i8* nonnull %arr8)
  ; Write Vec3.x (field 0,0) — byte offset 0
  %st0 = getelementptr inbounds [64 x %struct.Particle], [64 x %struct.Particle]* %arr, i64 0, i64 %idx, i32 0, i32 0
  store float 1.0, float* %st0, align 4
  ; Write Vec3.z (field 0,2) — byte offset 8
  %st1 = getelementptr inbounds [64 x %struct.Particle], [64 x %struct.Particle]* %arr, i64 0, i64 %idx, i32 0, i32 2
  store float 2.0, float* %st1, align 4
  ; Write .w (field 1) — byte offset 12
  %st2 = getelementptr inbounds [64 x %struct.Particle], [64 x %struct.Particle]* %arr, i64 0, i64 %idx, i32 1
  store float 3.0, float* %st2, align 4
  ; Read .id (field 2) — byte offset 16
  %ld0 = getelementptr inbounds [64 x %struct.Particle], [64 x %struct.Particle]* %arr, i64 0, i64 %idx, i32 2
  %id = load i32, i32* %ld0, align 4
  call void @llvm.lifetime.end.p0i8(i64 1280, i8* nonnull %arr8)
  %out = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 %id, i32 addrspace(1)* %out, align 4
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
