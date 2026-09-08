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
; The incoming value of a single-incoming pointer PHI may itself be a cast: the
; backward walk in tracesToAlloca has to look through the bitcast to reach the
; alloca.  Typed pointers are required to make the bitcast survive to the pass.
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%S = type { float, i32 }

; SoA: the 8-byte %S is split into two 4-byte chunks, the per-lane base strides
; by one chunk and every dynamic offset is scaled by the SIMD size.  The pointer
; PHI is walked through and disappears.
;
; CHECK-LABEL: define spir_kernel void @test_phi_of_bitcast(
; CHECK: [[LANE:%.*]] = zext i16 %{{.*}} to i32
; CHECK: [[SIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: mul i32 [[LANE]], 4
; CHECK: mul i32 [[SIMD]], %
; CHECK-NOT: phi i32*
; CHECK: ret void
define spir_kernel void @test_phi_of_bitcast(i32 addrspace(1)* nocapture %d, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) {
entry:
  %arr = alloca [64 x %S], align 8
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %i64 = zext i32 %i to i64
  %g = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %i64, i32 0
  store float 1.000000e+00, float* %g, align 4
  %bc = bitcast float* %g to i32*
  %inext = add nuw nsw i32 %i, 1
  %more = icmp ult i32 %inext, 64
  br i1 %more, label %loop, label %exit

exit:
  %p = phi i32* [ %bc, %loop ]
  %v = load i32, i32* %p, align 4
  store i32 %v, i32 addrspace(1)* %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8*)* @test_phi_of_bitcast}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8*)* @test_phi_of_bitcast, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
