;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; A pointer PHI with a single incoming value -- an LCSSA node left behind by loop
; simplification -- merges nothing: its block has exactly one predecessor, so the
; incoming pointer and the byte offset derived for it dominate every user. It is
; therefore walked through like a bitcast, and the alloca stays SoA-promotable
; even though the PHI feeds loads and stores rather than only comparisons (a real
; merge would first need one offset per incoming block to be merged).
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; sizeof(%SD) = 4 + 4 + 8*16 = 136 B, SOAPartitionBytes = 4.
%Closure = type { i32, float, float, float }
%SD = type { float, i32, [8 x %Closure] }

; The load behind the LCSSA PHI is transposed: per-lane stride is one 4-byte
; chunk, not the whole 136-byte object, and the PHI itself is gone.
;
; CHECK-LABEL: @test_lcssa_ptr_phi(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       mul i32 %{{.*}}, 4
; CHECK-NOT:   mul i32 %{{.*}}, 136
; CHECK:       [[BYTES:%.*]] = mul nsw i32 %{{.*}}, 16
; CHECK:       [[OFF:%.*]] = add i32 [[BYTES]], 8
; CHECK:       [[CHUNK:%.*]] = lshr i32 [[OFF]], 2
; CHECK:       mul i32 [[CHUNK]], 4
; CHECK:       load float, ptr %{{.*}}, align 4
; CHECK-NOT:   phi ptr
; CHECK:       ret void
define spir_kernel void @test_lcssa_ptr_phi(ptr addrspace(1) nocapture %d, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
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
  br i1 %more, label %loop, label %exit

exit:
  ; Single-incoming LCSSA PHI forwarding the last iteration's element pointer.
  %last = phi ptr [ %elt, %loop ]
  %v = load float, ptr %last, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

; Negative: a real merge of two incoming blocks whose result is loaded from. One
; offset per incoming block would have to be merged before the load could be
; transposed, so the alloca falls back to a contiguous per-lane AoS block
; (136 bytes rounded up to the alloca's 16-byte alignment).
;
; CHECK-LABEL: @test_two_arm_ptr_phi_negative(
; CHECK:       mul i32 %{{.*}}, 144
; CHECK:       ret void
define spir_kernel void @test_two_arm_ptr_phi_negative(ptr addrspace(1) nocapture %d, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %sd = alloca %SD, align 16
  %field = getelementptr inbounds %SD, ptr %sd, i64 0, i32 2
  %c = icmp sgt i32 %n, 0
  br i1 %c, label %then, label %else

then:
  %p0 = getelementptr inbounds [8 x %Closure], ptr %field, i64 0, i64 1
  br label %join

else:
  %p1 = getelementptr inbounds [8 x %Closure], ptr %field, i64 0, i64 3
  br label %join

join:
  %p = phi ptr [ %p0, %then ], [ %p1, %else ]
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
!4 = !{!"FuncMDMap[0]", ptr @test_lcssa_ptr_phi}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", ptr @test_two_arm_ptr_phi_negative}
!9 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test_lcssa_ptr_phi, !408}
!7 = !{ptr @test_two_arm_ptr_phi_negative, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
