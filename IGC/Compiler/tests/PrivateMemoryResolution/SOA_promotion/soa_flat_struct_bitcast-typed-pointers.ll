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
; Flat (scalar-leaf) promotion of a scalar array that is temporarily viewed as a
; struct: the alloca's base type is float, so visitBitCastInst must accept both
; halves of the round trip
;
;   bitcast [64 x float]* -> %Complex*   (scalar leaf -> struct)
;   bitcast %Complex*     -> float*      (struct -> type whose scalar size
;                                         matches the leaf)
;
; Typed pointers are required for these bitcasts to survive to the pass.
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%Complex = type { float, float, i32 }

; SoA: per-lane base strides by one float and both accesses are scaled by the
; SIMD size.  The bitcast chain is folded away into scratch address arithmetic.
;
; CHECK-LABEL: define spir_kernel void @test_flat_struct_bitcast(
; CHECK: [[LANE:%.*]] = zext i16 %{{.*}} to i32
; CHECK: [[SIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: mul i32 [[LANE]], 4
; CHECK: mul i32 [[SIMD]], 4
; CHECK: store float 1.000000e+00, float*
; CHECK: mul i32 [[SIMD]], 4
; CHECK: load float, float*
; CHECK-NOT: bitcast %Complex*
; CHECK: ret void
define spir_kernel void @test_flat_struct_bitcast(float addrspace(1)* nocapture %d, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) {
entry:
  %arr = alloca [64 x float], align 4
  %c = bitcast [64 x float]* %arr to %Complex*
  %f = bitcast %Complex* %c to float*
  store float 1.000000e+00, float* %f, align 4
  %idx = zext i32 %n to i64
  %g = getelementptr inbounds [64 x float], [64 x float]* %arr, i64 0, i64 %idx
  %v = load float, float* %g, align 4
  store float %v, float addrspace(1)* %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (float addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8*)* @test_flat_struct_bitcast}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{void (float addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8*)* @test_flat_struct_bitcast, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
