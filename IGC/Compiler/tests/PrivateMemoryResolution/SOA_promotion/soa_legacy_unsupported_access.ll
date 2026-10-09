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
; RUN:   --regkey EnablePrivMemNewSOATranspose=0 \
; RUN:   --igc-private-mem-resolution -S %s | FileCheck %s
;
; Check that unsupported accesses are rejected in the case of legacy SoA algorithm.

; Accesses within a lane are SoA-promoted: per-lane stride is 8 bytes.
;
; CHECK-LABEL: define spir_kernel void @in_lane_access(
; CHECK:       mul i32 %{{.*}}, 8
; CHECK-NOT:   getelementptr
; CHECK:       store <2 x i32> %v, ptr %{{.*}}, align 8
; CHECK-NOT:   getelementptr
; CHECK:       load <2 x i32>, ptr %{{.*}}, align 8
; CHECK:       ret void
define spir_kernel void @in_lane_access(ptr addrspace(1) %out, <2 x i32> %v, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [4 x <2 x i32>], align 8
  %p = getelementptr inbounds [4 x <2 x i32>], ptr %a, i32 0, i32 1
  store <2 x i32> %v, ptr %p, align 8
  %r = load <2 x i32>, ptr %p, align 8
  store <2 x i32> %r, ptr addrspace(1) %out, align 8
  ret void
}

; A 16-byte vector access spans two 8-byte lanes; reject SoA, AoS stride is 32 bytes.
;
; CHECK-LABEL: define spir_kernel void @wider_than_lane_access(
; CHECK:       mul i32 %{{.*}}, 32
; CHECK:       [[BASE:%.*]] = inttoptr i32 %{{.*}} to ptr
; CHECK:       [[P:%.*]] = getelementptr inbounds [4 x <2 x i32>], ptr [[BASE]], i32 0, i32 1
; CHECK:       store <4 x i32> %v, ptr [[P]], align 8
; CHECK:       load <4 x i32>, ptr [[P]], align 8
; CHECK:       ret void
define spir_kernel void @wider_than_lane_access(ptr addrspace(1) %out, <4 x i32> %v, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [4 x <2 x i32>], align 8
  %p = getelementptr inbounds [4 x <2 x i32>], ptr %a, i32 0, i32 1
  store <4 x i32> %v, ptr %p, align 8
  %r = load <4 x i32>, ptr %p, align 8
  store <4 x i32> %r, ptr addrspace(1) %out, align 16
  ret void
}

; Byte offset 4 is not aligned to the 8-byte lane; reject SoA, AoS stride is 32 bytes.
;
; CHECK-LABEL: define spir_kernel void @unaligned_to_lane_access(
; CHECK:       mul i32 %{{.*}}, 32
; CHECK:       [[BASE:%.*]] = inttoptr i32 %{{.*}} to ptr
; CHECK:       [[P:%.*]] = getelementptr inbounds i32, ptr [[BASE]], i32 1
; CHECK:       store <2 x i32> %v, ptr [[P]], align 8
; CHECK:       load <2 x i32>, ptr [[P]], align 8
; CHECK:       ret void
define spir_kernel void @unaligned_to_lane_access(ptr addrspace(1) %out, <2 x i32> %v, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [4 x <2 x i32>], align 8
  %p = getelementptr inbounds i32, ptr %a, i32 1
  store <2 x i32> %v, ptr %p, align 8
  %r = load <2 x i32>, ptr %p, align 8
  store <2 x i32> %r, ptr addrspace(1) %out, align 8
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21, !22}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !6, !7, !8, !9}
!4 = !{!"FuncMDMap[0]", ptr @in_lane_access}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{!"FuncMDMap[1]", ptr @wider_than_lane_access}
!7 = !{!"FuncMDValue[1]", !2}
!8 = !{!"FuncMDMap[2]", ptr @unaligned_to_lane_access}
!9 = !{!"FuncMDValue[2]", !2}
!20 = !{ptr @in_lane_access, !30}
!21 = !{ptr @wider_than_lane_access, !30}
!22 = !{ptr @unaligned_to_lane_access, !30}
!30 = !{!31, !32}
!31 = !{!"function_type", i32 0}
!32 = !{!"implicit_arg_desc", !33, !34, !35}
!33 = !{i32 0}
!34 = !{i32 1}
!35 = !{i32 13}
