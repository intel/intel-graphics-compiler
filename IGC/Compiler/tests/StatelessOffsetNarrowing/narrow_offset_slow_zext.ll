;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -o - --regkey EnableStatelessOffsetNarrowing=1 --regkey GreaterThan4GBBufferRequired=1 -igc-stateless-offset-narrowing | FileCheck %s

; This test verifies that stateless offset narrowing is applied even when
; GreaterThan4GBBufferRequired == true, provided that the max byte offset
; fits in u32. Here the index is zext i32 (activeBits=32, maxIdx=2^32-1)
; and element is i8 (1 byte), so maxOffset = (2^32-1)*1 = UINT32_MAX.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @narrow_offset_slow_zext(ptr addrspace(1) %src) {
entry:
  %gid = call i32 @get_global_id(i32 0)
  %gid64 = zext i32 %gid to i64
  %ptr = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %gid64

; CHECK-LABEL: @narrow_offset_slow_zext
; CHECK: %[[IDX32:.*]] = trunc i64 %gid64 to i32
; CHECK: %[[OFF32:.*]] = add i32 0, %[[IDX32]]
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(1) %src to i64
; CHECK: %[[OFF64:.*]] = zext i32 %[[OFF32]] to i64
; CHECK: %[[ADDR:.*]] = add i64 %[[BASE]], %[[OFF64]]
; CHECK: %[[PTR:.*]] = inttoptr i64 %[[ADDR]] to ptr addrspace(1)
; CHECK: load i8, ptr addrspace(1) %[[PTR]]
  %val = load i8, ptr addrspace(1) %ptr, align 1
  ret void
}

declare i32 @get_global_id(i32)

!IGCMetadata = !{!0}
!igc.functions = !{!10}

!0 = !{!"ModuleMD", !1, !8}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @narrow_offset_slow_zext}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7}
!7 = !{!"type", i32 0}
!8 = !{!"compOpt", !9}
!9 = !{!"GreaterThan4GBBufferRequired", i1 true}
!10 = !{ptr @narrow_offset_slow_zext, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
