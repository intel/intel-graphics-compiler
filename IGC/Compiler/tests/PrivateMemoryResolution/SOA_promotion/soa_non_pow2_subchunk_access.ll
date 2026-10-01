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
; Check that a sub-chunk access whose size is not a power of two rejects the
; alloca from the new SoA algorithm.
;
;
; CHECK-LABEL: define spir_kernel void @non_pow2_subchunk_access(
; CHECK:       mul i32 %{{.*}}, 32
; CHECK:       [[BASE:%.*]] = inttoptr i32 %{{.*}} to ptr
; CHECK-NEXT:  store <8 x i32> %init, ptr [[BASE]], align 32
; CHECK-NEXT:  [[P:%.*]] = getelementptr inbounds i32, ptr [[BASE]], i32 3
; CHECK-NEXT:  [[R:%.*]] = load <3 x i32>, ptr [[P]], align 4
; CHECK-NEXT:  store <3 x i32> [[R]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:  ret void
define spir_kernel void @non_pow2_subchunk_access(ptr addrspace(1) %out, <8 x i32> %init, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca <8 x i32>, align 32
  store <8 x i32> %init, ptr %a, align 32
  %p = getelementptr inbounds i32, ptr %a, i32 3
  %r = load <3 x i32>, ptr %p, align 4
  store <3 x i32> %r, ptr addrspace(1) %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @non_pow2_subchunk_access}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{ptr @non_pow2_subchunk_access, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 13}
