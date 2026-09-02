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
; RUN: igc_opt --typed-pointers --ocl --platformpvc --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1  -S %s | FileCheck %s
;
; Test SoA promotion for array of structs alloca.
; Test models workloads sub-struct write pattern where a float3 field
; is stored as a single <3 x i32> write via bitcast.
;
; A [4 x %struct.Color] alloca is accessed via a bitcast Color* → <3 x i32>*
; and then stored with a 12-byte vector store.
;
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Color = type { float, float, float }

; CHECK-LABEL: @test_vec_store
;;
;; SoA prolog
;;
; CHECK:       [[T00:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:       [[simdLaneId:%.*]] = zext i16 [[T00]] to i32
; CHECK:       [[simdSize:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       [[T01:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
;;
;; The 12-byte vector store is split into three 4-byte SoA chunk stores,
;; each using "mul i32 simdSize, ..." to compute the per-lane byte offset.
;;
; CHECK:       [[C0:%.*]] = mul i32 [[simdSize]], {{.*}}
; CHECK:       {{store i32.*}}
; CHECK:       [[C1:%.*]] = mul i32 [[simdSize]], {{.*}}
; CHECK:       {{store i32.*}}
; CHECK:       [[C2:%.*]] = mul i32 [[simdSize]], {{.*}}
; CHECK:       {{store i32.*}}

; Function Attrs: nofree nosync nounwind
define spir_kernel void @test_vec_store(i32 addrspace(1)* nocapture writeonly %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i64 1
  %pb = alloca [4 x %struct.Color], align 4
  %tmp0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %tmp1 = add i32 %tmp0, %localIdX3
  %ix = add i32 %tmp1, %payloadHeader.scalar
  %idx = zext i32 %ix to i64

  ; Access element %idx as a whole Color struct via bitcast to <3 x i32>*
  ; and store the struct in one 12-byte vector write.
  %staddr = getelementptr inbounds [4 x %struct.Color], [4 x %struct.Color]* %pb, i64 0, i64 %idx
  %bc     = bitcast %struct.Color* %staddr to <3 x i32>*
  store <3 x i32> zeroinitializer, <3 x i32>* %bc, align 4

  ; Use the alloca result so PMR does not discard it
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 0, i32 addrspace(1)* %arrayidx, align 4

; CHECK: ret
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_vec_store}
!5 = !{!"FuncMDValue[1]", !2, !432}
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_vec_store, !408}
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
!418 = !{!"argId", i32 0}
!419 = !{!"implicitArgInfoListVec[0]", !418}
!420 = !{!"argId", i32 1}
!421 = !{!"implicitArgInfoListVec[1]", !420}
!422 = !{!"argId", i32 7}
!423 = !{!"implicitArgInfoListVec[2]", !422}
!424 = !{!"argId", i32 8}
!425 = !{!"implicitArgInfoListVec[3]", !424}
!426 = !{!"argId", i32 9}
!427 = !{!"implicitArgInfoListVec[4]", !426}
!428 = !{!"argId", i32 10}
!429 = !{!"implicitArgInfoListVec[5]", !428}
!430 = !{!"argId", i32 13}
!431 = !{!"implicitArgInfoListVec[6]", !430}
!432 = !{!"implicitArgInfoList", !419, !421, !423, !425, !427, !429, !431}
