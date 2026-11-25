;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 \
; RUN:   --regkey EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 \
; RUN:   --regkey EnableAggressiveSOAPromotion=0 -S %s | FileCheck %s

; Check that 2nd operand of icmp comparison is correctly checked for null and non-null cases
; is correctly checked for inbound.

define spir_kernel void @kernel(ptr addrspace(1) %out, i64 %offset, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
  %array = alloca [16 x float], align 4
  call void @callee(ptr %array, i64 %offset, ptr addrspace(1) %out)
  ret void
}

; CHECK-LABEL: define internal void @callee(
; CHECK: [[WRAP:%.*]] = getelementptr float, ptr %base, i64 %offset
; CHECK: [[PTR:%.*]] = getelementptr inbounds float, ptr [[WRAP]], i32 0
; CHECK: [[EQ:%.*]] = icmp eq ptr null, [[PTR]]
; CHECK: [[NE:%.*]] = icmp ne ptr [[PTR]], null
; CHECK: select i1 [[EQ]], i32 1, i32 0
; CHECK: select i1 [[NE]], i32 2, i32 0
define internal void @callee(ptr %base, i64 %offset, ptr addrspace(1) %out) {
  store float 1.0, ptr %base, align 4
  %wrapped = getelementptr float, ptr %base, i64 %offset
  %candidate = getelementptr inbounds float, ptr %wrapped, i32 0
  %isnull = icmp eq ptr null, %candidate
  %nonnull = icmp ne ptr %candidate, null
  %first = select i1 %isnull, i32 1, i32 0
  %second = select i1 %nonnull, i32 2, i32 0
  %result = add i32 %first, %second
  store i32 %result, ptr addrspace(1) %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!3, !10}
!0 = !{!"ModuleMD", !1, !14}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 false}
!3 = !{ptr @kernel, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 13}
!10 = !{ptr @callee, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 1}
!14 = !{!"FuncMD", !15, !16}
!15 = !{!"FuncMDMap[0]", ptr @kernel}
!16 = !{!"FuncMDValue[0]", !17}
!17 = !{!"implicitArgInfoList", !18, !19, !20}
!18 = !{!"implicitArgInfoListVec[0]", !21}
!19 = !{!"implicitArgInfoListVec[1]", !22}
!20 = !{!"implicitArgInfoListVec[2]", !23}
!21 = !{!"argId", i32 0}
!22 = !{!"argId", i32 1}
!23 = !{!"argId", i32 13}
