;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test verifies if snap_wa is effectively disabled for SPV_INTEL_bindless_images extension.

; The snap_wa workaround is disabled for bindless images from the SPV_INTEL_bindless_images extension.
; This is because the current implementation of the workaround requires the sampler to be known at compile-time,
; either as an inline sampler or a kernel argument. This allows the UMD to program the
; SAMPLER_SNAP_WA implicit argument, which indicates whether the workaround should be enabled.
;
; For bindless images from SPV_INTEL_bindless_images, image is represented as an i64 handle (bindlessOffset)
; provided by the user. The handle is a runtime value and cannot be tracked to a kernel argument at compile-time.
; Therefore, implementing snap_wa would require a completely new approach.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%spirv.Sampler = type opaque

declare i32 @__builtin_IB_get_snap_wa_reqd(i32)

define i32 @foo(i32 %sampler_handle) nounwind {
; CHECK-NOT:     call i32 @__builtin_IB_get_snap_wa_reqd(i32 %sampler_handle)
  %id = call i32 @__builtin_IB_get_snap_wa_reqd(i32 %sampler_handle)
; CHECK:         ret i32 0
  ret i32 %id
}

!IGCMetadata = !{!0}
!igc.functions = !{!3}

!0 = !{!"ModuleMD", !1}
!1 = !{!"extensions", !2}
!2 = !{!"spvINTELBindlessImages", i1 true}
!3 = !{i32 (i32)* @foo, !1}
!4 = !{ptr @foo, !5}
!5 = !{!5, !6}
!6 = !{!"function_type", i32 0}
