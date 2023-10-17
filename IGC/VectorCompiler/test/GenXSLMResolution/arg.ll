;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXSLMResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define dllexport void @kernel(i32 addrspace(3)* align 1 %out, i32 addrspace(3)* nocapture readonly %in) local_unnamed_addr #0 {
entry:
  ; CHECK: %ld = load i32, i32 addrspace(3)* %in, align 4
  %ld = load i32, i32 addrspace(3)* %in, align 4

  ; CHECK: store i32 %ld, i32 addrspace(3)* inttoptr (i32 12 to i32 addrspace(3)*), align 4
  store i32 %ld, i32 addrspace(3)* %out, align 4
  ret void
}

attributes #0 = { nofree noinline norecurse nounwind "CMGenxMain" "oclrt"="1" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32 addrspace(3)*, i32 addrspace(3)*)* @kernel, !"kernel", !1, i32 11, !2, !3, !4, i32 0}
!1 = !{i32 64, i32 128}
!2 = !{i32 -1, i32 16}
!3 = !{i32 0}
!4 = !{}
!5 = !{void (i32 addrspace(3)*, i32 addrspace(3)*)* @kernel, !6, !7, !8, null}
!6 = !{i32 0, i32 0}
!7 = !{i32 0, i32 0}
!8 = !{!9}
!9 = !{i32 0, !10}
!10 = !{!11}
!11 = !{i32 1, i32 0}
