;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl \
; RUN:   --regkey EnablePrivMemNewSOAForScalarArrays=1 \
; RUN:   --igc-private-mem-resolution -S %s | FileCheck %s
;
; Check SoA promotion doesn't access addresses past the allocated reservation.
;
; selectPartitionSize() clamps P up to 4, so this affects any sub-dword array
; whose byte size is not a multiple of 4. Such an alloca cannot be laid out in
; SoA at all and must fall back to AoS.

; [255 x half] = 510 bytes, chunk 127 starts at byte 508 and
; its column runs to 512 * simdSize, overflowing the 510 * simdSize reservation
; by 2 * simdSize - bytes.
;
; CHECK-LABEL: define spir_kernel void @odd_size_half_array(
; CHECK: mul i32 %{{.*}}, 510
; CHECK: getelementptr inbounds half
;; getChunkNum() emits an lshr by log2(P); its absence proves TransposePrivMem
;; did not lower this alloca.
; CHECK-NOT: lshr
define spir_kernel void @odd_size_half_array(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %stack = alloca [255 x half], align 2
  %idx = zext i32 %ix to i64
  %elt = getelementptr inbounds half, ptr %stack, i64 %idx
  store half 0xH3C00, ptr %elt, align 2
  %v = load half, ptr %elt, align 2
  store half %v, ptr addrspace(1) %d, align 2
  ret void
}

; Control: [256 x half] = 512 bytes is exactly 128 chunks, so the same shape is
; still SoA-promoted - the guard rejects only the partial tail chunk, not every
; sub-dword array.
;
; CHECK-LABEL: define spir_kernel void @whole_chunk_half_array(
; CHECK: lshr
; CHECK-NOT: mul i32 %{{.*}}, 512
define spir_kernel void @whole_chunk_half_array(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %stack = alloca [256 x half], align 2
  %idx = zext i32 %ix to i64
  %elt = getelementptr inbounds half, ptr %stack, i64 %idx
  store half 0xH3C00, ptr %elt, align 2
  %v = load half, ptr %elt, align 2
  store half %v, ptr addrspace(1) %d, align 2
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !6, !7}
!4 = !{!"FuncMDMap[0]", ptr @odd_size_half_array}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{!"FuncMDMap[1]", ptr @whole_chunk_half_array}
!7 = !{!"FuncMDValue[1]", !2}
!20 = !{ptr @odd_size_half_array, !408}
!21 = !{ptr @whole_chunk_half_array, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
