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
; The array is accessed through a private->generic (addrspace 4) round-trip.
; The SoA buffer lives in private memory, so the resolved scratch pointer must be
; a private pointer (inttoptr ... to float*), not a generic one (float addrspace(4)*).
;
; RUN: igc_opt --typed-pointers -regkey EnablePrivMemNewSOAForScalarArrays=0 --ocl --platformPtl --igc-private-mem-resolution -S %s | FileCheck %s

; CHECK-LABEL: @test(
; The scratch offset must be reinterpreted as a private pointer, never generic AS4.
; CHECK-NOT:   inttoptr i32 {{.*}} to float addrspace(4)*
; CHECK:       inttoptr i32 {{.*}} to float*
; CHECK:       store float
; CHECK-NOT:   inttoptr i32 {{.*}} to float addrspace(4)*
; CHECK:       inttoptr i32 {{.*}} to float*
; CHECK:       load float

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(float addrspace(1)* nocapture writeonly %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i64 1
  %pb = alloca [33 x float], align 4
  %tmp0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %tmp1 = add i32 %tmp0, %localIdX3
  %ix = add i32 %tmp1, %payloadHeader.scalar
  %idx = zext i32 %ix to i64
  %staddr = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx
  %staddr.g = addrspacecast float* %staddr to float addrspace(4)*
  %ix_f = sitofp i32 %ix to float
  store float %ix_f, float addrspace(4)* %staddr.g
  %tmp3 = add i32 %ix, 1
  %tmp4 = urem i32 %tmp3, 33
  %idx1 = zext i32 %tmp4 to i64
  %ldaddr = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx1
  %ldaddr.g = addrspacecast float* %ldaddr to float addrspace(4)*
  %v0 = load float, float addrspace(4)* %ldaddr.g, align 4
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %d, i64 %idx
  store float %v0, float addrspace(1)* %arrayidx, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[1]", void (float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test}
!5 = !{!"FuncMDValue[1]", !2}
!6 = !{void (float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test, !408}
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
