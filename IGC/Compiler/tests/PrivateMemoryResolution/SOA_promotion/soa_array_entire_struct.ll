;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; No SOA transpose
; RUN: igc_opt --ocl --platformpvc --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=2  -S %s | FileCheck --check-prefix=CHECK-K2 %s
;
; SOA transpose on the entire struct
; RUN: igc_opt --ocl --platformpvc --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=3  -S %s | FileCheck --check-prefix=CHECK-K3 %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Packed = type <{ i8, i16, i8, float }>

; CHECK-LABEL: @test
;;
;; prolog in entry block. Get buffer's perThreadOffset
;;
; CHECK-K2:       [[T00:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK-K2:       [[simdLaneId:%.*]] = zext i16 [[T00]] to i32
; CHECK-K2:       [[simdSize:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK-K2:       [[T01:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK-K2:       [[T02:%.*]] = mul i32 [[simdSize]], 8192
; CHECK-K2:       [[perThreadOffset:%.*]] = mul i32 [[T01]], [[T02]]
;
;; No SOA transpose
;;
; CHECK-K2:       {{.*}} = mul i32 [[simdLaneId]], 8192
;
;;
;;
; CHECK-K3:       [[T00:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK-K3:       [[simdLaneId:%.*]] = zext i16 [[T00]] to i32
; CHECK-K3:       [[simdSize:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK-K3:       [[T01:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK-K3:       [[T02:%.*]] = mul i32 [[simdSize]], 8192
; CHECK-K3:       [[perThreadOffset:%.*]] = mul i32 [[T01]], [[T02]]
;
;; SOA transpose for the entire packed struct
;;
; CHECK-K3:       {{.*}} = mul i32 [[simdLaneId]], 8

; Function Attrs: nofree nosync nounwind
define spir_kernel void @test(i32 addrspace(1)* nocapture writeonly %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i64 1
  %pb = alloca [1024 x %struct.Packed ], align 4
  %tmp0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %tmp1 = add i32 %tmp0, %localIdX3
  %ix = add i32 %tmp1, %payloadHeader.scalar
  %idx = zext i32 %ix to i64
  %tmp2 = bitcast [1024 x %struct.Packed ]* %pb to i8*
  call void @llvm.lifetime.start.p0i8(i64 8192, i8* nonnull %tmp2)
  %staddr0 = getelementptr inbounds [1024 x %struct.Packed ], [1024 x %struct.Packed ]* %pb, i64 0, i64 %idx, i32 1
  store i16 1, i16* %staddr0, align 1
  %staddr1 = getelementptr inbounds [1024 x %struct.Packed ], [1024 x %struct.Packed ]* %pb, i64 0, i64 %idx, i32 3
  store float 0.000000e+00, float* %staddr1, align 4
  call void @llvm.lifetime.end.p0i8(i64 8192, i8* nonnull %tmp2)
;
; CHECK:   ret
  ret void
}

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
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
!413 = !{i32 6}
!414 = !{i32 7}
!415 = !{i32 8}
!416 = !{i32 9}
!417 = !{i32 12}
