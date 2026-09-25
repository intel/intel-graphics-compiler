;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; A chain of single-incoming (LCSSA) pointer PHIs must be walked through
; transparently: tracesToAlloca has to recurse through the PHI operand of the
; outer PHI to prove that both nodes stay inside the same alloca.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=0 -S %s \
; RUN:   | FileCheck %s --check-prefix=NOAGGR

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%Closure = type { i32, float, float, float }
%SD = type { float, i32, [8 x %Closure] }

; Heterogeneous struct reached through two chained LCSSA PHIs.
;
; SoA: per-lane base strides by one 4-byte chunk and every offset is scaled by
; the SIMD size; both pointer PHIs are gone.
;
; CHECK-LABEL: define spir_kernel void @test_chained_lcssa_phi(
; CHECK: [[LANE:%.*]] = zext i16 %{{.*}} to i32
; CHECK: [[SIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: mul i32 [[LANE]], 4
; CHECK: mul i32 [[SIMD]], %
; CHECK-NOT: phi ptr
;
; AoS: per-lane base strides by the whole 144-byte (16-aligned) %SD and the
; pointer PHIs survive.
;
; NOAGGR-LABEL: define spir_kernel void @test_chained_lcssa_phi(
; NOAGGR: mul i32 %{{.*}}, 144
; NOAGGR: phi ptr
; NOAGGR: phi ptr
define spir_kernel void @test_chained_lcssa_phi(ptr addrspace(1) nocapture %d, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %sd = alloca %SD, align 16
  %field = getelementptr inbounds %SD, ptr %sd, i64 0, i32 2
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %i64 = zext i32 %i to i64
  %elt = getelementptr inbounds [8 x %Closure], ptr %field, i64 0, i64 %i64
  store float 1.000000e+00, ptr %elt, align 4
  %inext = add nuw nsw i32 %i, 1
  %more = icmp ult i32 %inext, 8
  br i1 %more, label %loop, label %exit1

exit1:
  %p1 = phi ptr [ %elt, %loop ]
  br label %exit2

exit2:
  %p2 = phi ptr [ %p1, %exit1 ]
  %v = load float, ptr %p2, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

; Same regkey gate on a plain scalar array, where the struct-specific paths of
; the checker are not involved at all.
;
; CHECK-LABEL: define spir_kernel void @test_scalar_array_lcssa_phi(
; CHECK: [[LANE2:%.*]] = zext i16 %{{.*}} to i32
; CHECK: [[SIMD2:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: mul i32 [[LANE2]], 4
; CHECK: mul i32 [[SIMD2]], 4
; CHECK-NOT: phi ptr
; CHECK: ret void
;
; NOAGGR-LABEL: define spir_kernel void @test_scalar_array_lcssa_phi(
; NOAGGR: mul i32 %{{.*}}, 256
; NOAGGR: phi ptr
define spir_kernel void @test_scalar_array_lcssa_phi(ptr addrspace(1) nocapture %d, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %arr = alloca [64 x float], align 4
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %i64 = zext i32 %i to i64
  %elt = getelementptr inbounds [64 x float], ptr %arr, i64 0, i64 %i64
  store float 1.000000e+00, ptr %elt, align 4
  %inext = add nuw nsw i32 %i, 1
  %more = icmp ult i32 %inext, 64
  br i1 %more, label %loop, label %exit

exit:
  %p = phi ptr [ %elt, %loop ]
  %v = load float, ptr %p, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !8, !9}
!4 = !{!"FuncMDMap[0]", ptr @test_chained_lcssa_phi}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", ptr @test_scalar_array_lcssa_phi}
!9 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test_chained_lcssa_phi, !408}
!7 = !{ptr @test_scalar_array_lcssa_phi, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
