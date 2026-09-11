;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -o - --regkey EnableStatelessOffsetNarrowing=1 -igc-stateless-offset-narrowing | FileCheck %s

; This test verifies how the constant part of a GEP chain offset is handled.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @constant_offset_not_narrowed(ptr addrspace(1) align 4 %src) {
entry:
  %ptr = getelementptr inbounds float, ptr addrspace(1) %src, i64 16

; CHECK-LABEL: @constant_offset_not_narrowed
; CHECK-NOT: base.i64
; CHECK-NOT: narrow.address
; CHECK-NOT: narrow.ptr
  %val = load float, ptr addrspace(1) %ptr, align 4
  ret void
}

define spir_kernel void @constant_part_stays_on_base(ptr addrspace(2) align 8 %constBase) {
entry:
  %tbl = getelementptr i8, ptr addrspace(2) %constBase, i64 728
  %gid = call i32 @get_global_id(i32 0)
  %gid64 = zext i32 %gid to i64
  %ptr = getelementptr inbounds [32 x i64], ptr addrspace(2) %tbl, i64 0, i64 %gid64

; CHECK-LABEL: @constant_part_stays_on_base
; CHECK: %[[IDX32:.*]] = trunc i64 %gid64 to i32
; CHECK: %[[MUL:.*]] = mul i32 8, %[[IDX32]]
; CHECK-NOT: add i32 728
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(2) %constBase to i64
; CHECK: %[[BASEOFF:.*]] = add i64 %[[BASE]], 728
; CHECK: %[[OFF64:.*]] = zext i32 %[[MUL]] to i64
; CHECK: %[[ADDR:.*]] = add i64 %[[BASEOFF]], %[[OFF64]]
; CHECK: %[[PTR:.*]] = inttoptr i64 %[[ADDR]] to ptr addrspace(2)
; CHECK: load i64, ptr addrspace(2) %[[PTR]]
  %val = load i64, ptr addrspace(2) %ptr, align 8
  ret void
}

declare i32 @get_global_id(i32)

!IGCMetadata = !{!0}
!igc.functions = !{!10, !13}

!0 = !{!"ModuleMD", !1, !8}
!1 = !{!"FuncMD", !2, !3, !16, !17}
!2 = !{!"FuncMDMap[0]", ptr @constant_offset_not_narrowed}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7}
!7 = !{!"type", i32 0}
!8 = !{!"compOpt", !9}
!9 = !{!"GreaterThan4GBBufferRequired", i1 false}
!10 = !{ptr @constant_offset_not_narrowed, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
!13 = !{ptr @constant_part_stays_on_base, !14}
!14 = !{!15}
!15 = !{!"function_type", i32 0}
!16 = !{!"FuncMDMap[1]", ptr @constant_part_stays_on_base}
!17 = !{!"FuncMDValue[1]", !18}
!18 = !{!"resAllocMD", !19}
!19 = !{!"argAllocMDList", !6}
