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
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; Allocas passed to calls that cannot be promoted must keep the per-lane (AoS)
; layout, and their callees must be left alone.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-NOT: = alloca
;; %a_ok: promoted. The lane offset is simdLaneId * 4 (SoA partition).
; CHECK: mul i32 {{%.*}}, 4
; CHECK: call void @good(ptr {{%.*}})
;; %a_rec, %a_mixed, %a_off: AoS, the lane offset is simdLaneId * 256.
; CHECK: mul i32 {{%.*}}, 256
; CHECK: call void @recursive(ptr {{%.*}}, i32 3)
; CHECK: mul i32 {{%.*}}, 256
; CHECK: call void @mixed(ptr {{%.*}})
; CHECK: call void @mixed(ptr %privateBase)
; CHECK: mul i32 {{%.*}}, 256
; CHECK: call void @offset(ptr {{%.*}})

; CHECK-LABEL: define internal void @good(
; CHECK: ptrtoint ptr %p to i64

; CHECK-LABEL: define internal void @recursive(
; CHECK-NOT: ptrtoint
; CHECK: store float 1.000000e+00, ptr %p

; CHECK-LABEL: define internal void @mixed(
; CHECK-NOT: ptrtoint
; CHECK: store float 1.000000e+00, ptr %p

; CHECK-LABEL: define internal void @offset(
; CHECK-NOT: ptrtoint
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a_ok = alloca [64 x float], align 4
  %a_rec = alloca [64 x float], align 4
  %a_mixed = alloca [64 x float], align 4
  %a_off = alloca [64 x float], align 4
  call void @good(ptr %a_ok)
  call void @recursive(ptr %a_rec, i32 3)
  call void @mixed(ptr %a_mixed)
  call void @mixed(ptr %privateBase)
  %shifted = getelementptr inbounds float, ptr %a_off, i32 4
  call void @offset(ptr %shifted)
  ret void
}

define internal void @good(ptr %p) {
  store float 1.0, ptr %p, align 4
  ret void
}

define internal void @recursive(ptr %p, i32 %n) {
  store float 1.0, ptr %p, align 4
  %c = icmp eq i32 %n, 0
  br i1 %c, label %done, label %again
again:
  %m = sub i32 %n, 1
  call void @recursive(ptr %p, i32 %m)
  br label %done
done:
  ret void
}

define internal void @mixed(ptr %p) {
  store float 1.0, ptr %p, align 4
  ret void
}

define internal void @offset(ptr %p) {
  store float 1.0, ptr %p, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!500, !520, !521, !522, !523}
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
!520 = !{ptr @good, !450}
!521 = !{ptr @recursive, !450}
!522 = !{ptr @mixed, !450}
!523 = !{ptr @offset, !450}
