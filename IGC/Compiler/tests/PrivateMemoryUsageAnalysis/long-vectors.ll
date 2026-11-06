;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --igc-private-mem-usage-analysis -igc-add-implicit-args -platformmtl -ocl -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --igc-private-mem-usage-analysis -igc-add-implicit-args -platformmtl -ocl -regkey DisableLongVectorsOpt=1 -S < %s | FileCheck %s --check-prefix=NO-LVO

; LongVectorsOpt runs after this analysis and may create allocas for vectors
; over LongVectorsOptThreshold (128 B), so such functions get the private base.

; CHECK: define spir_kernel void @long_vec(ptr addrspace(1) %p, <8 x i32> %r0, ptr %privateBase)
; NO-LVO: define spir_kernel void @long_vec(ptr addrspace(1) %p, <8 x i32> %r0)
define spir_kernel void @long_vec(ptr addrspace(1) %p) {
  %v = load <64 x i32>, ptr addrspace(1) %p, align 4
  %r = add <64 x i32> %v, %v
  store <64 x i32> %r, ptr addrspace(1) %p, align 4
  ret void
}

; CHECK: define spir_kernel void @short_vec(ptr addrspace(1) %p, <8 x i32> %r0)
; NO-LVO: define spir_kernel void @short_vec(ptr addrspace(1) %p, <8 x i32> %r0)
define spir_kernel void @short_vec(ptr addrspace(1) %p) {
  %v = load <16 x i32>, ptr addrspace(1) %p, align 4
  store <16 x i32> %v, ptr addrspace(1) %p, align 4
  ret void
}

; CHECK: define spir_func <64 x i32> @long_vec_args(<64 x i32> %a, <64 x i32> %b, <8 x i32> %r0, ptr %privateBase)
; NO-LVO: define spir_func <64 x i32> @long_vec_args(<64 x i32> %a, <64 x i32> %b, <8 x i32> %r0)
define spir_func <64 x i32> @long_vec_args(<64 x i32> %a, <64 x i32> %b) {
  %r = add <64 x i32> %a, %b
  ret <64 x i32> %r
}

!igc.functions = !{!0, !3, !4}
!0 = !{ptr @long_vec, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @short_vec, !1}
!4 = !{ptr @long_vec_args, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 1}
