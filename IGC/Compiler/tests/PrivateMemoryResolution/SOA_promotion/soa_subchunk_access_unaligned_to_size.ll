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
; RUN:   --regkey EnableAggressiveSOAPromotion=1 \
; RUN:   --regkey EnableSOAFallbackToOldAlgorithm=0 \
; RUN:   --igc-private-mem-resolution -S %s | FileCheck %s
;
; Check that a sub-chunk access whose intra-chunk offset is not a multiple of
; its own size is not SoA-promoted by the new algorithm.
;
;
; CHECK-LABEL: define spir_kernel void @subchunk_unaligned(
; CHECK: mul i32 %{{.*}}, 16
; CHECK-NOT: getelementptr <2 x i8>
; CHECK: [[P:%.*]] = getelementptr [16 x i8], ptr %{{.*}}, i32 0, i32 1
; CHECK-NOT: getelementptr <2 x i8>
; CHECK: store <2 x i8> <i8 7, i8 9>, ptr [[P]]
; CHECK-NOT: getelementptr <2 x i8>
; CHECK: load <2 x i8>, ptr [[P]]
; CHECK-NOT: getelementptr <2 x i8>
; CHECK: ret void
define spir_kernel void @subchunk_unaligned(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [16 x i8], align 4
  %p = getelementptr [16 x i8], ptr %a, i32 0, i32 1
  store <2 x i8> <i8 7, i8 9>, ptr %p, align 1
  %v = load <2 x i8>, ptr %p, align 1
  store <2 x i8> %v, ptr addrspace(1) %d, align 2
  ret void
}

; CHECK-LABEL: define spir_kernel void @subchunk_aligned(
; CHECK: mul i32 %{{.*}}, 4
; CHECK-NOT: mul i32 %{{.*}}, 16
; CHECK: [[SP:%.*]] = getelementptr <2 x i8>, ptr %{{.*}}, i32 1
; CHECK: store <2 x i8> <i8 7, i8 9>, ptr [[SP]]
; CHECK: [[LP:%.*]] = getelementptr <2 x i8>, ptr %{{.*}}, i32 1
; CHECK: load <2 x i8>, ptr [[LP]]
define spir_kernel void @subchunk_aligned(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [16 x i8], align 4
  %p = getelementptr [16 x i8], ptr %a, i32 0, i32 2
  store <2 x i8> <i8 7, i8 9>, ptr %p, align 2
  %v = load <2 x i8>, ptr %p, align 2
  store <2 x i8> %v, ptr addrspace(1) %d, align 2
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !6, !7}
!4 = !{!"FuncMDMap[0]", ptr @subchunk_unaligned}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{!"FuncMDMap[1]", ptr @subchunk_aligned}
!7 = !{!"FuncMDValue[1]", !2}
!20 = !{ptr @subchunk_unaligned, !408}
!21 = !{ptr @subchunk_aligned, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
