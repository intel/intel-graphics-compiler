;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --ocl --igc-private-mem-resolution --platformpvc \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1 --regkey EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

; Check that structs with dynamic member type GEPs are handled correctly by the new SoA promotion and transpose logic.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; sizeof(%Closure) = 16 B, the closure array starts at 16 B, one closure into %SD.
%Closure = type { i32, float, float, float }
%SD = type { [4 x float], [8 x %Closure] }

; sizeof(%Odd) = 6 B, not a multiple of the 4-byte partition.
%Odd = type { i16, i16, i16 }
%SDOdd = type { [3 x i16], [8 x %Odd] }

; %sd is SoA: the per-lane stride is one 4-byte chunk and the dynamic byte offset is split into a chunk number.
; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK:       [[LANE:%.*]] = zext i16 {{%.*}} to i32
; CHECK:       mul i32 [[LANE]], 4
; CHECK:       [[BYTES:%.*]] = mul nsw i32 {{%.*}}, 16
; CHECK:       [[OFF:%.*]] = add i32 4, [[BYTES]]
; CHECK:       lshr i32 [[OFF]], 2
; CHECK:       store float 3.500000e+00
; CHECK:       load float
; %sdo is not: its per-lane stride is the whole 54-byte object (rounded up to its 8-byte alignment).
; CHECK:       mul i32 [[LANE]], 56
; CHECK:       ret void
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %sd = alloca %SD, align 16
  %sdo = alloca %SDOdd, align 8
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  %idx = zext i32 %i to i64
  %k = add nuw nsw i64 %idx, 1
  %elt = getelementptr inbounds %Closure, ptr %sd, i64 %k
  %f = getelementptr inbounds i8, ptr %elt, i64 4
  store float 3.5, ptr %f, align 4
  %v = load float, ptr %f, align 4
  store float %v, ptr addrspace(1) %d, align 4
  %elto = getelementptr inbounds %Odd, ptr %sdo, i64 %k
  store i16 7, ptr %elto, align 2
  %vo = load i16, ptr %elto, align 2
  %out = getelementptr inbounds i8, ptr addrspace(1) %d, i64 4
  store i16 %vo, ptr addrspace(1) %out, align 2
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!500}
!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 false}
!3 = !{!"FuncMD", !10, !11}
!10 = !{!"FuncMDMap[0]", ptr @kernel}
!11 = !{!"FuncMDValue[0]", !20}
!20 = !{!"implicitArgInfoList", !21, !22, !23, !24, !25, !26, !27}
!21 = !{!"implicitArgInfoListVec[0]", !30}
!22 = !{!"implicitArgInfoListVec[1]", !31}
!23 = !{!"implicitArgInfoListVec[2]", !32}
!24 = !{!"implicitArgInfoListVec[3]", !33}
!25 = !{!"implicitArgInfoListVec[4]", !34}
!26 = !{!"implicitArgInfoListVec[5]", !35}
!27 = !{!"implicitArgInfoListVec[6]", !36}
!30 = !{!"argId", i32 0}
!31 = !{!"argId", i32 1}
!32 = !{!"argId", i32 7}
!33 = !{!"argId", i32 8}
!34 = !{!"argId", i32 9}
!35 = !{!"argId", i32 10}
!36 = !{!"argId", i32 13}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !413, !414, !415, !416, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
!450 = !{!451}
!451 = !{!"function_type", i32 1}
!500 = !{ptr @kernel, !408}
