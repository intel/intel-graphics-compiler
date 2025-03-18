;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --typed-pointers --ocl --platformpvc --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=3  -S %s | FileCheck %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Pixel = type { i8, i8, i8, i8 }

; CHECK-LABEL: @test
;;
;; prolog in entry block. Get buffer's perThreadOffset
;;
; CHECK:       [[T00:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[simdLaneId:%.*]] = zext i16 [[T00]] to i32
; CHECK:       [[simdSize:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       [[T01:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK:       [[T02:%.*]] = mul i32 [[simdSize]], 2048
; CHECK:       [[perThreadOffset:%.*]] = mul i32 [[T01]], [[T02]]
;

; Function Attrs: nofree nosync nounwind
define spir_kernel void @test(i32 addrspace(1)* nocapture writeonly %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i64 1
  %pb = alloca [512 x %struct.Pixel], align 4
  %tmp0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %tmp1 = add i32 %tmp0, %localIdX3
  %ix = add i32 %tmp1, %payloadHeader.scalar
;;
;;
; CHECK:       %idx = zext i32 %ix to i64
; CHECK:       [[T10_0:%.*]] = mul i32 [[simdSize]], 0
; CHECK:       [[T10:%.*]] = add i32 0, [[T10_0]]
; CHECK:       [[T11:%.*]] = mul i32 [[simdLaneId]], 4
; CHECK:       [[T12:%.*]] = add i32 [[T10]], [[T11]]
; CHECK:       [[bufferOffset:%.*]] = add {{.*}} i32 [[perThreadOffset]], [[T12]]
; CHECK:       [[T13:%.*]] = ptrtoint i8* %privateBase to i64
; CHECK:       [[T14:%.*]] = zext i32 [[bufferOffset]] to i64
; CHECK:       [[bufbaseAsInt:%.*]] = add {{.*}} i64 [[T13]], [[T14]]
;;
;;   store of the 1st struct member
;;
; CHECK:       [[T20:%.*]] = trunc i64 %idx to i32
; CHECK:       [[T21:%.*]] = mul nsw i32 [[T20]], 4
; CHECK:       [[T22:%.*]] = lshr i32 [[T21]], 2
; CHECK:       [[T23:%.*]] = mul i32 [[T22]], 4
; CHECK:       [[T24:%.*]] = mul i32 [[simdSize]], [[T23]]
; CHECK:       [[T25:%.*]] = zext i32 [[T24]] to i64
; CHECK:       [[T26:%.*]] = add {{.*}} i64 [[bufbaseAsInt]], [[T25]]
; CHECK:       [[i0:%.*]]  = and i32 [[T21]], 3
; CHECK:       [[GEPBase:%.*]] = inttoptr i64 [[T26]] to i8*
; CHECK:       [[staddr0:%.*]] = getelementptr i8, i8* [[GEPBase]], i32 [[i0]]
; CHECK:       store i8 1, i8* [[staddr0]]
;;
;; store of the remaining three i8 members
;;
; CHECK:       [[T30:%.*]] = trunc i64 %idx to i32
; CHECK:       [[T31:%.*]] = mul nsw i32 [[T30]], 4
; CHECK:       [[T32:%.*]] = add nsw i32 [[T31]], 1
; CHECK:       [[T33:%.*]] = lshr i32 [[T32]], 2
; CHECK:       {{.*}} = getelementptr
; CHECK:       store i8 2,
;
; CHECK:       [[T40:%.*]] = trunc i64 %idx to i32
; CHECK:       [[T41:%.*]] = mul nsw i32 [[T40]], 4
; CHECK:       [[T42:%.*]] = add nsw i32 [[T41]], 2
; CHECK:       [[T43:%.*]] = lshr i32 [[T42]], 2
; CHECK:       {{.*}} = getelementptr
; CHECK:       store i8 3,
;
; CHECK:       [[T50:%.*]] = trunc i64 %idx to i32
; CHECK:       [[T51:%.*]] = mul nsw i32 [[T50]], 4
; CHECK:       [[T52:%.*]] = add nsw i32 [[T51]], 3
; CHECK:       [[T53:%.*]] = lshr i32 [[T52]], 2
; CHECK:       {{.*}} = getelementptr
; CHECK:       store i8 4,
;
  %idx = zext i32 %ix to i64
  %tmp2 = bitcast [512 x %struct.Pixel]* %pb to i8*
  call void @llvm.lifetime.start.p0i8(i64 2048, i8* nonnull %tmp2)
  %staddr0 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx, i32 0
  store i8 1, i8* %staddr0, align 4
  %staddr1 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx, i32 1
  store i8 2, i8* %staddr1, align 1
  %staddr2 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx, i32 2
  store i8 3, i8* %staddr2, align 2
  %staddr3 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx, i32 3
  store i8 4, i8* %staddr3, align 1
;;
;; 4 loads, same patterns as store
;;
; CHECK:     %idx1 = add i64 %idx, 1
; CHECK:     [[T61:%.*]] = mul i32 [[simdSize]],
; CHECK:     [[T71:%.*]] = mul i32 [[simdSize]],
; CHECK:     [[T81:%.*]] = mul i32 [[simdSize]],
;
  %idx1 = add i64 %idx, 1
  %ldaddr0 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx1, i32 0
  %v0 = load i8, i8* %ldaddr0, align 4
  %ldaddr1 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx1, i32 1
  %v1 = load i8, i8* %ldaddr1, align 1
  %ldaddr2 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx1, i32 2
  %v2 = load i8, i8* %ldaddr2, align 2
  %ldaddr3 = getelementptr inbounds [512 x %struct.Pixel], [512 x %struct.Pixel]* %pb, i64 0, i64 %idx1, i32 3
  %v3 = load i8, i8* %ldaddr3, align 1
  %v4 = add i8 %v0, %v1
  %v5 = add i8 %v4, %v2
  %v6 = add i8 %v5, %v3
  %v7 = zext i8 %v6 to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 %v7, i32 addrspace(1)* %arrayidx, align 4
  call void @llvm.lifetime.end.p0i8(i64 2048, i8* nonnull %tmp2)
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
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
