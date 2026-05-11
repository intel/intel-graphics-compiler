;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -o - --regkey EnableStatelessOffsetNarrowing=1 --regkey GreaterThan4GBBufferRequired=1 -igc-stateless-offset-narrowing | FileCheck %s

; This test verifies that stateless offset narrowing is NOT applied when
; GreaterThan4GBBufferRequired == true and the max byte offset cannot be
; proven to fit in u32. The index is zext i32 (activeBits=32) and element
; is float (4 bytes), so maxOffset = (2^32-1)*4 > UINT32_MAX.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @narrow_offset_slow_no_narrow(ptr addrspace(1) align 4 %src) {
entry:
  %gid = call i32 @get_global_id(i32 0)
  %gid64 = zext i32 %gid to i64
  %ptr = getelementptr inbounds float, ptr addrspace(1) %src, i64 %gid64
; CHECK-LABEL: @narrow_offset_slow_no_narrow
; CHECK-NOT: narrow.ptr
; CHECK-NOT: narrow.addr
  %val = load float, ptr addrspace(1) %ptr, align 4
  ret void
}

declare i32 @get_global_id(i32)

!IGCMetadata = !{!0}
!igc.functions = !{!10}

!0 = !{!"ModuleMD", !1, !8}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @narrow_offset_slow_no_narrow}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7}
!7 = !{!"type", i32 0}
!8 = !{!"compOpt", !9}
!9 = !{!"GreaterThan4GBBufferRequired", i1 true}
!10 = !{ptr @narrow_offset_slow_no_narrow, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
