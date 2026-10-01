;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --igc-mem-access-throttle -platformCri -regkey MemAccessThrottle=1 -S < %s | FileCheck %s
; ------------------------------------------------
; MemAccessThrottle - a hand-written throttle is left alone
; ------------------------------------------------

; A hand-written throttle is left alone: the pattern matches, but the kernel
; already paces itself, so a second throttle would change that tuning.
define spir_kernel void @manual_throttle(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                         ptr addrspace(1) %out2, <8 x i32> %r0) {
; CHECK-LABEL: @manual_throttle(
; CHECK:         call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    ret void
entry:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl, ptr addrspace(1) %out2, align 16
  call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  ret void
}

declare void @llvm.genx.GenISA.eu.thread.pause(i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()

!igc.functions = !{!0}
!IGCMetadata = !{!20}

!0 = !{ptr @manual_throttle, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!10 = !{!"argId", i32 0}
!11 = !{!"implicitArgInfoListVec[0]", !10}
!12 = !{!"implicitArgInfoList", !11}
!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !22, !23}
!22 = !{!"FuncMDMap[0]", ptr @manual_throttle}
!23 = !{!"FuncMDValue[0]", !12}
