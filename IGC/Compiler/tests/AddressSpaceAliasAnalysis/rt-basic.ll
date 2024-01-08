;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; COM: Due to different aa options naming
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt %s -S -aa-eval -igc-raytracing-address-space-alias-analysis -igc-aa-wrapper  \
; RUN: -disable-basic-aa -print-all-alias-modref-info -disable-output 2>&1 | FileCheck %s
; ------------------------------------------------
; RayTracingAddressSpaceAAWrapperPass
; ------------------------------------------------

; COM: A test to check IGCExternalAAWrapper(igc-aa-wrapper) with
; COM: RayTracingAddressSpaceAAWrapperPass analysis pass


; COM: Not an RT Addrspaces, MayAlias expected
; CHECK:  MayAlias:     i32 addrspace(23)* %{{.*}}, i32 addrspace(24)* %{{.*}}
define void @f0(i32 addrspace(23)* %src, i32 addrspace(24)* %src1) {
entry:
  store i32 13, i32 addrspace(23)* %src
  store i32 14, i32 addrspace(24)* %src1
  ret void
}

; COM: non-equal RT and non-RT Addrspaces, NoAlias expected
; CHECK:  NoAlias:     i32 addrspace(23)* %{{.*}}, i32 addrspace(26)* %{{.*}}
define void @f1(i32 addrspace(23)* %src, i32 addrspace(26)* %src1) {
entry:
  store i32 13, i32 addrspace(23)* %src
  store i32 14, i32 addrspace(26)* %src1
  ret void
}

; COM: non-equal RT Addrspaces, NoAlias expected
; CHECK:  NoAlias:     i32 addrspace(25)* %{{.*}}, i32 addrspace(26)* %{{.*}}
define void @f2(i32 addrspace(25)* %src, i32 addrspace(26)* %src1) {
entry:
  store i32 13, i32 addrspace(25)* %src
  store i32 14, i32 addrspace(26)* %src1
  ret void
}

; COM: equal RT Addrspaces, MayAlias expected
; CHECK:  MayAlias:     i32 addrspace(26)* %{{.*}}, i32 addrspace(26)* %{{.*}}
define void @f3(i32 addrspace(26)* %src, i32 addrspace(26)* %src1) {
entry:
  store i32 13, i32 addrspace(26)* %src
  store i32 14, i32 addrspace(26)* %src1
  ret void
}

!igc.functions = !{!0, !3, !4, !5}

!0 = !{void (i32 addrspace(23)*, i32 addrspace(24)*)* @f0, !1}
!3 = !{void (i32 addrspace(23)*, i32 addrspace(26)*)* @f1, !1}
!4 = !{void (i32 addrspace(25)*, i32 addrspace(26)*)* @f2, !1}
!5 = !{void (i32 addrspace(26)*, i32 addrspace(26)*)* @f3, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}


!IGCMetadata = !{!10}

!10 = !{!"ModuleMD", !14}
!14 = !{!"rtInfo", !15, !16}
!15 = !{!"SWHotZoneAddrspace", i32 25}
!16 = !{!"SWStackAddrspace", i32 26}

