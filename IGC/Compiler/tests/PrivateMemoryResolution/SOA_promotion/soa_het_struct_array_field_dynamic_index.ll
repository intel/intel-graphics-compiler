;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; A heterogeneous struct alloca must stay SoA-promotable when an inner
; array-of-struct field is indexed dynamically and the element is then accessed
; directly, i.e. the parent GEP's result element type is the *struct*:
;     %e = getelementptr [N x %Closure], ptr %field, i64 0, i64 %idx
;     %v = load float, ptr %e
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; sizeof(%Closure) = 16 B, a multiple of SOAPartitionBytes = 4.
%Closure = type { i32, float, float, float }
; sizeof(%SD) = 4 + 4 + 8*16 = 136 B.
%SD = type { float, i32, [8 x %Closure] }

; sizeof(%Odd) = 2 B, not a multiple of the partition.
%Odd = type { i8, i8 }
; sizeof(%SDOdd) = 4 + 4 + 8*2 = 24 B.
%SDOdd = type { float, i32, [8 x %Odd] }

; The dynamic element offset (%idx * 16, plus the field base 8) is turned into a
; chunk number, so the per-lane stride is one 4-byte chunk rather than the whole
; 136-byte object.
;
; CHECK-LABEL: @test_array_field_dynamic_index(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       mul i32 %{{.*}}, 4
; CHECK-NOT:   mul i32 %{{.*}}, 136
; CHECK:       [[BYTES:%.*]] = mul nsw i32 %{{.*}}, 16
; CHECK:       [[OFF:%.*]] = add i32 [[BYTES]], 8
; CHECK:       [[CHUNK:%.*]] = lshr i32 [[OFF]], 2
; CHECK:       mul i32 [[CHUNK]], 4
; CHECK:       load float, ptr %{{.*}}, align 4
; CHECK:       ret void
define spir_kernel void @test_array_field_dynamic_index(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %sd = alloca %SD, align 16
  %idx = zext i32 %ix to i64
  %field = getelementptr inbounds %SD, ptr %sd, i64 0, i32 2
  ; Dynamic index over %Closure: the element type of the parent GEP is a struct,
  ; but the access below it covers exactly one chunk.
  %elt = getelementptr inbounds [8 x %Closure], ptr %field, i64 0, i64 %idx
  store float 3.500000e+00, ptr %elt, align 4
  %v = load float, ptr %elt, align 4
  %out = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

; Negative: %Odd is 2 bytes, so consecutive dynamic indices land at alternating
; intra-chunk offsets and no single chunk number describes the access. The alloca
; must fall back to a contiguous per-lane AoS block (24 bytes rounded up to the
; alloca's 16-byte alignment).
;
; CHECK-LABEL: @test_array_field_dynamic_index_odd_stride_negative(
; CHECK:       mul i32 %{{.*}}, 32
; CHECK:       ret void
define spir_kernel void @test_array_field_dynamic_index_odd_stride_negative(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %sd = alloca %SDOdd, align 16
  %idx = zext i32 %ix to i64
  %field = getelementptr inbounds %SDOdd, ptr %sd, i64 0, i32 2
  %elt = getelementptr inbounds [8 x %Odd], ptr %field, i64 0, i64 %idx
  store i32 7, ptr %elt, align 2
  %v = load i32, ptr %elt, align 2
  %out = getelementptr inbounds i32, ptr addrspace(1) %d, i64 %idx
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !8, !9}
!4 = !{!"FuncMDMap[0]", ptr @test_array_field_dynamic_index}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", ptr @test_array_field_dynamic_index_odd_stride_negative}
!9 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test_array_field_dynamic_index, !408}
!7 = !{ptr @test_array_field_dynamic_index_odd_stride_negative, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
