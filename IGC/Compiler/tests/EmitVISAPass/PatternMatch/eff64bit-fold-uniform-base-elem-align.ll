;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformCri -simd-mode 32 -igc-emit-visa -regkey DumpVISAASMToConsole -regkey EnableEfficient64b < %s | FileCheck %s

; Element aligned vector load: the base goes into IND0, no 64-bit add of base + offset.
; CHECK-LABEL: .kernel "elem_align_v4"
; CHECK-NOT:   add (M1, 32)
; CHECK:       lsc_load.ugm (M1, 32)  x:d32x4  flat(q_0v)[shl0]:a64

define spir_kernel void @elem_align_v4(ptr addrspace(1) %q, ptr addrspace(1) %dst, i16 %localIdX) {
entry:
  %base = ptrtoint ptr addrspace(1) %q to i64
  %idx = zext i16 %localIdX to i64
  %shl0 = shl nuw nsw i64 %idx, 2
  %addr = add i64 %shl0, %base
  %ptr = inttoptr i64 %addr to ptr addrspace(1)
  %x = load <4 x float>, ptr addrspace(1) %ptr, align 4
  store <4 x float> %x, ptr addrspace(1) %dst, align 16
  ret void
}

; Same for a 3-element vector.
; CHECK-LABEL: .kernel "elem_align_v3"
; CHECK-NOT:   add (M1, 32)
; CHECK:       lsc_load.ugm (M1, 32)  x:d32x3  flat(q_0v)[shl0]:a64

define spir_kernel void @elem_align_v3(ptr addrspace(1) %q, ptr addrspace(1) %dst, i16 %localIdX) {
entry:
  %base = ptrtoint ptr addrspace(1) %q to i64
  %idx = zext i16 %localIdX to i64
  %shl0 = shl nuw nsw i64 %idx, 2
  %addr = add i64 %shl0, %base
  %ptr = inttoptr i64 %addr to ptr addrspace(1)
  %x = load <3 x float>, ptr addrspace(1) %ptr, align 4
  store <3 x float> %x, ptr addrspace(1) %dst, align 16
  ret void
}

; Below element alignment: the base is not folded, the full address is used.
; CHECK-LABEL: .kernel "below_elem_align"
; CHECK:       add (M1, 32)  addr(0,0)<1> shl0(0,0)<1;1,0> q_0v(0,0)<0;1,0>
; CHECK:       lsc_load.ugm (M1, 32)  x:d32  flat[addr_0v]:a64

define spir_kernel void @below_elem_align(ptr addrspace(1) %q, ptr addrspace(1) %dst, i16 %localIdX) {
entry:
  %base = ptrtoint ptr addrspace(1) %q to i64
  %idx = zext i16 %localIdX to i64
  %shl0 = shl nuw nsw i64 %idx, 2
  %addr = add i64 %shl0, %base
  %ptr = inttoptr i64 %addr to ptr addrspace(1)
  %x = load <4 x float>, ptr addrspace(1) %ptr, align 2
  store <4 x float> %x, ptr addrspace(1) %dst, align 16
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21, !22}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !4, !5, !6, !7}
!2 = !{!"FuncMDMap[0]", ptr @elem_align_v4}
!3 = !{!"FuncMDValue[0]", !10, !30}
!4 = !{!"FuncMDMap[1]", ptr @elem_align_v3}
!5 = !{!"FuncMDValue[1]", !10, !30}
!6 = !{!"FuncMDMap[2]", ptr @below_elem_align}
!7 = !{!"FuncMDValue[2]", !10, !30}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12, !13, !14}
!12 = !{!"argAllocMDListVec[0]"}
!13 = !{!"argAllocMDListVec[1]"}
!14 = !{!"argAllocMDListVec[2]"}
!20 = !{ptr @elem_align_v4, !23}
!21 = !{ptr @elem_align_v3, !23}
!22 = !{ptr @below_elem_align, !23}
!23 = !{!24}
!24 = !{!"function_type", i32 0}
!30 = !{!"implicitArgInfoList", !31}
!31 = !{!"implicitArgInfoListVec[0]", !32}
!32 = !{!"argId", i32 8}
