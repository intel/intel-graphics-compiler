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
; RUN:   --regkey EnableAggressiveSOAPromotion=1 \
; RUN:   --igc-private-mem-resolution -S %s | FileCheck %s
;
; The aggressive struct-SoA acceptance rules admit a chunk-sized access reached
; through a "member access" GEP. That decision looks at the parent-level GEP
; alone, so an unaligned constant contributed by any *earlier* link of the
; pointer chain reaches TransposePrivMem unchecked - and there
; getTransposedEltPtr() lowers a chunk-wide access with eltIx = 0, i.e. on the
; assumption that it starts at intra-chunk offset 0.
;
; SOALayoutChecker::isPartitionAlignedChain() therefore walks the whole chain
; back to the alloca instead of trusting one link.

%S = type { [4 x i16], i32 }   ; 12 bytes, P = 4

; %base lands on byte 38 (element 3 of the array, then i16 index 1 inside the
; first member: 36 + 2), so the i32 load at %q is at byte 42 - intra-chunk
; offset 2, straddling chunks 10 and 11. No SoA: the per-lane base advances by
; the whole 504-byte array and the original GEPs survive.
;
; CHECK-LABEL: define spir_kernel void @chain_unaligned(
; CHECK: mul i32 %{{.*}}, 504
; CHECK: getelementptr inbounds i32
;; getChunkNum() emits an lshr by log2(P); its absence proves TransposePrivMem
;; did not lower this alloca.
; CHECK-NOT: lshr
define spir_kernel void @chain_unaligned(ptr addrspace(1) nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %s = alloca [42 x %S], align 8
  %base = getelementptr inbounds [42 x %S], ptr %s, i64 0, i64 3, i32 0, i64 1
  %q = getelementptr inbounds i32, ptr %base, i64 1
  %v = load i32, ptr %q, align 2
  store i32 %v, ptr addrspace(1) %d, align 4
  ret void
}

; Control: the same chain with i16 index 0 puts %base on byte 36 and %q on byte
; 40, both partition-aligned, so the access is still SoA-promoted - the guard
; rejects only the unaligned chain, not the member-access pattern.
;
; CHECK-LABEL: define spir_kernel void @chain_aligned(
; CHECK: mul i32 %{{.*}}, 4
; CHECK-NOT: mul i32 %{{.*}}, 504
define spir_kernel void @chain_aligned(ptr addrspace(1) nocapture %d, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %s = alloca [42 x %S], align 8
  %base = getelementptr inbounds [42 x %S], ptr %s, i64 0, i64 3, i32 0, i64 0
  %q = getelementptr inbounds i32, ptr %base, i64 1
  %v = load i32, ptr %q, align 4
  store i32 %v, ptr addrspace(1) %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !6, !7}
!4 = !{!"FuncMDMap[0]", ptr @chain_unaligned}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{!"FuncMDMap[1]", ptr @chain_aligned}
!7 = !{!"FuncMDValue[1]", !2}
!20 = !{ptr @chain_unaligned, !408}
!21 = !{ptr @chain_aligned, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
