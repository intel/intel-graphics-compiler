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
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 \
; RUN:   --igc-private-mem-resolution -S %s | FileCheck %s
;
; Check that an access at a non-partition-aligned offset doesn't lose its intra-chunk bytes
;
; SOALayoutChecker must prove the whole pointer chain partition-aligned before
; accepting such an access, since each visitor only ever sees one link of it.

; [64 x i16] with P = 4. The <2 x i16> access is exactly one chunk wide, but
; element 3 puts it at byte 6, i.e. intra-chunk offset 2, straddling chunks 1
; and 2. No SoA: the per-lane base advances by the whole 128-byte array and the
; original GEP survives.
;
; CHECK-LABEL: define spir_kernel void @unaligned_chunk_access(
; CHECK: mul i32 %{{.*}}, 128
; CHECK: getelementptr inbounds i16
;; getChunkNum() emits an lshr by log2(P); its absence proves TransposePrivMem
;; did not lower this alloca.
; CHECK-NOT: lshr
define spir_kernel void @unaligned_chunk_access(ptr addrspace(1) nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %stack = alloca [64 x i16], align 8
  %elt = getelementptr inbounds i16, ptr %stack, i64 3
  store <2 x i16> <i16 7, i16 9>, ptr %elt, align 2
  %v = load <2 x i16>, ptr %elt, align 2
  store <2 x i16> %v, ptr addrspace(1) %d, align 4
  ret void
}

; Control: element 2 is byte 4, exactly chunk 1 at intra-chunk offset 0, so the
; same access is still SoA-promoted - the guard rejects only the unaligned
; offset, not chunk-wide accesses in general.
;
; CHECK-LABEL: define spir_kernel void @aligned_chunk_access(
; CHECK: mul i32 %{{.*}}, 4
; CHECK-NOT: mul i32 %{{.*}}, 128
define spir_kernel void @aligned_chunk_access(ptr addrspace(1) nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %stack = alloca [64 x i16], align 8
  %elt = getelementptr inbounds i16, ptr %stack, i64 2
  store <2 x i16> <i16 7, i16 9>, ptr %elt, align 4
  %v = load <2 x i16>, ptr %elt, align 4
  store <2 x i16> %v, ptr addrspace(1) %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !6, !7}
!4 = !{!"FuncMDMap[0]", ptr @unaligned_chunk_access}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{!"FuncMDMap[1]", ptr @aligned_chunk_access}
!7 = !{!"FuncMDValue[1]", !2}
!20 = !{ptr @unaligned_chunk_access, !408}
!21 = !{ptr @aligned_chunk_access, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
