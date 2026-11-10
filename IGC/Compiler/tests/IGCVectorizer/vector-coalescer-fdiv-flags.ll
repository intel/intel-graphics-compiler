;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer -dce < %s | FileCheck %s

; fast + no flags
; CHECK-LABEL: define spir_kernel void @fast_and_plain(
; CHECK: = fdiv {{.*}}<16 x float>
; CHECK: ret void

define spir_kernel void @fast_and_plain(<8 x float> %a, <8 x float> %b, <8 x float> %c, <8 x float> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %div0 = fdiv fast <8 x float> %a, %b
  %div1 = fdiv <8 x float> %c, %d
  store <8 x float> %div0, ptr addrspace(1) %p0
  store <8 x float> %div1, ptr addrspace(1) %p1
  ret void
}

; nnan arcp + arcp afn
; CHECK-LABEL: define spir_kernel void @partial_overlap(
; CHECK: = fdiv {{.*}}<16 x float>
; CHECK: ret void

define spir_kernel void @partial_overlap(<8 x float> %a, <8 x float> %b, <8 x float> %c, <8 x float> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %div0 = fdiv nnan arcp <8 x float> %a, %b
  %div1 = fdiv arcp afn <8 x float> %c, %d
  store <8 x float> %div0, ptr addrspace(1) %p0
  store <8 x float> %div1, ptr addrspace(1) %p1
  ret void
}

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !3}

!0 = !{ptr @fast_and_plain, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @partial_overlap, !1}

!10 = !{!"requiredSubGroupSize", i32 16}
!11 = !{!"FuncMDMap[0]", ptr @fast_and_plain}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"FuncMDMap[1]", ptr @partial_overlap}
!14 = !{!"FuncMDValue[1]", !10}
!15 = !{!"FuncMD", !11, !12, !13, !14}
!16 = !{!"ModuleMD", !15}
!IGCMetadata = !{!16}
