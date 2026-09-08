;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; A scalar array alloca must stay SoA-promotable when a byte-granular GEP is
; chained onto an element GEP. InstCombine canonicalizes `&stack[i + 2]` into
;     %e = getelementptr float, ptr %stack, i64 %i
;     %b = getelementptr i8, ptr %e, i64 8
; Such a byte GEP never matches the base scalar size, but its offset lands on a
; whole element, and both transpose helpers lower it byte-precisely, so the walk
; must continue through it instead of vetoing the alloca.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnablePrivMemNewSOAForScalarArrays=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; The per-lane stride is one element (4 B), not the whole 1020-byte buffer, and
; the +8 byte offset becomes +2 chunks scaled by simdSize.
;
; CHECK-LABEL: @test_element_byte_gep(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       mul i32 %{{.*}}, 4
; CHECK-NOT:   mul i32 %{{.*}}, 1020
; CHECK:       [[BYTES:%.*]] = mul nsw i32 %{{.*}}, 4
; CHECK:       [[OFF:%.*]] = add i32 8, [[BYTES]]
; CHECK:       [[CHUNK:%.*]] = lshr i32 [[OFF]], 2
; CHECK:       mul i32 [[CHUNK]], 4
; CHECK:       store float 3.500000e+00, ptr %{{.*}}, align 4
; CHECK:       ret void
define spir_kernel void @test_element_byte_gep(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %stack = alloca [255 x float], align 4
  %idx = zext i32 %ix to i64
  %elt = getelementptr inbounds float, ptr %stack, i64 %idx
  ; +8 bytes == +2 floats: an offset onto a whole element.
  %byte = getelementptr inbounds i8, ptr %elt, i64 8
  store float 3.500000e+00, ptr %byte, align 4
  %v = load float, ptr %byte, align 4
  %out = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

; Negative: +2 bytes splits an element in half. The byte offset is not a
; multiple of the 4-byte element, so the alloca must fall back to a contiguous
; per-lane AoS block of 1020 bytes.
;
; CHECK-LABEL: @test_element_byte_gep_misaligned_negative(
; CHECK:       mul i32 %{{.*}}, 1020
; CHECK:       ret void
define spir_kernel void @test_element_byte_gep_misaligned_negative(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %stack = alloca [255 x float], align 4
  %idx = zext i32 %ix to i64
  %elt = getelementptr inbounds float, ptr %stack, i64 %idx
  %byte = getelementptr inbounds i8, ptr %elt, i64 2
  store float 3.500000e+00, ptr %byte, align 2
  %v = load float, ptr %byte, align 2
  %out = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !8, !9}
!4 = !{!"FuncMDMap[0]", ptr @test_element_byte_gep}
!5 = !{!"FuncMDValue[0]", !2}
!8 = !{!"FuncMDMap[1]", ptr @test_element_byte_gep_misaligned_negative}
!9 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test_element_byte_gep, !408}
!7 = !{ptr @test_element_byte_gep_misaligned_negative, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
