;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -o - --regkey EnableStatelessOffsetNarrowing=1 -igc-stateless-offset-narrowing | FileCheck %s

; HasPositivePointerOffset guarantees only that the total offset is non-negative,
; so an individual index may be negative. The constant part must then stay in the
; 32-bit accumulation, because hoisting it onto the 64-bit base would drop the
; borrow from a negative variable part across the zext.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @constant_stays_in_offset_when_index_may_be_negative(ptr addrspace(1) align 4 %src) {
entry:
  %idx = call i32 @get_index()
  %idx64 = sext i32 %idx to i64
  %tbl = getelementptr i8, ptr addrspace(1) %src, i64 64
  %ptr = getelementptr inbounds float, ptr addrspace(1) %tbl, i64 %idx64

; CHECK-LABEL: @constant_stays_in_offset_when_index_may_be_negative
; CHECK: %[[IDX32:.*]] = trunc i64 %idx64 to i32
; CHECK: %[[MUL:.*]] = mul i32 4, %[[IDX32]]
; CHECK: %[[OFF32:.*]] = add i32 %[[MUL]], 64
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(1) %src to i64
; CHECK: %[[OFF64:.*]] = zext i32 %[[OFF32]] to i64
; CHECK: %[[ADDR:.*]] = add i64 %[[BASE]], %[[OFF64]]
; CHECK: %[[PTR:.*]] = inttoptr i64 %[[ADDR]] to ptr addrspace(1)
; CHECK: load float, ptr addrspace(1) %[[PTR]]
; CHECK-NOT: base.offset.i64
  %val = load float, ptr addrspace(1) %ptr, align 4
  ret void
}

declare i32 @get_index()

!IGCMetadata = !{!0}
!igc.functions = !{!11}

!0 = !{!"ModuleMD", !1, !8}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @constant_stays_in_offset_when_index_may_be_negative}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7}
!7 = !{!"type", i32 0}
!8 = !{!"compOpt", !9, !10}
!9 = !{!"GreaterThan4GBBufferRequired", i1 false}
!10 = !{!"HasPositivePointerOffset", i1 true}
!11 = !{ptr @constant_stays_in_offset_when_index_may_be_negative, !12}
!12 = !{!13}
!13 = !{!"function_type", i32 0}
