;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; A scalar array alloca must stay SoA when it is indexed through byte-array GEPs.
; InstCombine canonicalizes `&P[i]` into
;     %p = getelementptr [4 x i8], ptr %P, i64 %i
; and `&P[i + 1]` into an i8 GEP of +4 bytes on top of it. A 4-byte step is one
; element, so both transpose helpers can address it; a 2-byte step splits an
; element and must fall back to the per-lane AoS block of 72 bytes.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnablePrivMemNewSOAForScalarArrays=1 -S %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,NEW
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOAForScalarArrays=0 -S %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,OLD

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; The per-lane stride is one element (4 B), not the whole 72-byte array.
; The new algorithm splits the byte offset i * 4 into chunk i; the old one
; scales the index i itself, not i * 4. P[i + 1] is one element further.
;
; CHECK-LABEL: @test_lane_stride(
; CHECK:       mul i32 %{{.*}}, 4
; CHECK-NOT:   mul i32 %{{.*}}, 72
; CHECK:       [[I:%.*]] = trunc i64 %idx to i32
; NEW-NEXT:    [[BYTES:%.*]] = mul nsw i32 [[I]], 4
; NEW-NEXT:    lshr i32 [[BYTES]], 2
; OLD-NEXT:    [[A:%.*]] = add i32 0, [[I]]
; OLD-NEXT:    [[ELT:%.*]] = add i32 [[A]], 0
; CHECK:       store i32 %l, ptr
; NEW:         [[NEXT:%.*]] = add i32 4, [[BYTES]]
; NEW-NEXT:    lshr i32 [[NEXT]], 2
; OLD:         add i32 1, [[ELT]]
; CHECK:       store i32 %r, ptr
; CHECK:       ret void
define spir_kernel void @test_lane_stride(ptr addrspace(1) nocapture %d, i32 %ix, i32 %l, i32 %r, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %P = alloca [18 x i32], align 4
  %idx = zext i32 %ix to i64
  %p = getelementptr inbounds [4 x i8], ptr %P, i64 %idx
  store i32 %l, ptr %p, align 4
  %q = getelementptr inbounds i8, ptr %p, i64 4
  store i32 %r, ptr %q, align 4
  %v = load i32, ptr %p, align 4
  %out = getelementptr inbounds i32, ptr addrspace(1) %d, i64 %idx
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

; Negative: a 2-byte step splits an element, so the alloca keeps the AoS layout.
;
; CHECK-LABEL: @test_split_lane_negative(
; CHECK:       mul i32 %{{.*}}, 72
; CHECK:       getelementptr inbounds [2 x i8]
; CHECK:       ret void
define spir_kernel void @test_split_lane_negative(ptr addrspace(1) nocapture %d, i32 %ix, i32 %l, i32 %r, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %P = alloca [18 x i32], align 4
  %idx = zext i32 %ix to i64
  %p = getelementptr inbounds [2 x i8], ptr %P, i64 %idx
  store i32 %l, ptr %p, align 2
  %v = load i32, ptr %p, align 2
  %out = getelementptr inbounds i32, ptr addrspace(1) %d, i64 %idx
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

; GEP indices are signed. A vector element type keeps both configurations on the
; legacy transpose helper, which indexes whole vectors: i8 -1 from vector 2 is
; vector 1, not vector 257, and a dynamic i8 index is sign-extended.
;
; CHECK-LABEL: @test_vector_narrow_negative(
; CHECK:       mul i32 1, %{{.*}}
; CHECK:       store <4 x i32> <i32 42
; CHECK-NOT:   mul i32 257,
; CHECK:       mul i32 1, %{{.*}}
; CHECK:       load <4 x i32>
; CHECK:       ret void
define spir_kernel void @test_vector_narrow_negative(ptr addrspace(1) %out, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [8 x <4 x i32>], align 16
  %v1 = getelementptr inbounds [8 x <4 x i32>], ptr %a, i64 0, i64 1
  store <4 x i32> <i32 42, i32 43, i32 44, i32 45>, ptr %v1, align 16
  %v2 = getelementptr inbounds [8 x <4 x i32>], ptr %a, i64 0, i64 2
  %p = getelementptr inbounds [16 x i8], ptr %v2, i8 -1
  %v = load <4 x i32>, ptr %p, align 16
  store <4 x i32> %v, ptr addrspace(1) %out, align 16
  ret void
}

; CHECK-LABEL: @test_vector_narrow_dynamic(
; CHECK-NOT:   zext i8 %i
; CHECK:       sext i8 %i to i32
; CHECK:       ret void
define spir_kernel void @test_vector_narrow_dynamic(ptr addrspace(1) %out, i8 %i, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [8 x <4 x i32>], align 16
  %v1 = getelementptr inbounds [8 x <4 x i32>], ptr %a, i64 0, i64 1
  store <4 x i32> <i32 42, i32 43, i32 44, i32 45>, ptr %v1, align 16
  %v2 = getelementptr inbounds [8 x <4 x i32>], ptr %a, i64 0, i64 2
  %p = getelementptr inbounds [16 x i8], ptr %v2, i8 %i
  %v = load <4 x i32>, ptr %p, align 16
  store <4 x i32> %v, ptr addrspace(1) %out, align 16
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7, !12, !13}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !8, !9, !10, !11, !14, !15}
!4 = !{!"FuncMDMap[0]", ptr @test_lane_stride}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", ptr @test_split_lane_negative}
!9 = !{!"FuncMDValue[1]", !2}
!10 = !{!"FuncMDMap[2]", ptr @test_vector_narrow_negative}
!11 = !{!"FuncMDValue[2]", !2}
!14 = !{!"FuncMDMap[3]", ptr @test_vector_narrow_dynamic}
!15 = !{!"FuncMDValue[3]", !2}
!6 = !{ptr @test_lane_stride, !408}
!7 = !{ptr @test_split_lane_negative, !408}
!12 = !{ptr @test_vector_narrow_negative, !408}
!13 = !{ptr @test_vector_narrow_dynamic, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
