;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer -dce < %s 2>&1 | FileCheck %s

; identical instructions are folded before pairing

; exact copy
; CHECK-LABEL: @exact
; CHECK: %coalesced_input = shufflevector <8 x float> %x, <8 x float> %y, <16 x i32>
; CHECK: %coalesced_intrinsic = call <16 x float> @llvm.exp2.v16f32(<16 x float> %coalesced_input)
; CHECK: %coalesced_output = shufflevector <16 x float> %coalesced_intrinsic, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK: %coalesced_output1 = shufflevector <16 x float> %coalesced_intrinsic, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: %s1 = fmul <8 x float> %coalesced_output, %coalesced_output
; CHECK: %s2 = fmul <8 x float> %s1, %coalesced_output1
; CHECK-NOT: coalesced
; CHECK: ret void
define spir_kernel void @exact(ptr addrspace(1) %out, <8 x float> %x, <8 x float> %y) #0 {
entry:
  %e1 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %x)
  %e2 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %x)
  %e3 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %y)
  %s1 = fmul <8 x float> %e1, %e2
  %s2 = fmul <8 x float> %s1, %e3
  store <8 x float> %s2, ptr addrspace(1) %out, align 32
  ret void
}

; commuted copy
; CHECK-LABEL: @commuted
; CHECK: %coalesced_input = shufflevector <8 x float> %a, <8 x float> %a, <16 x i32>
; CHECK: %coalesced_input1 = shufflevector <8 x float> %b, <8 x float> %c, <16 x i32>
; CHECK: %coalesced_binary = fmul <16 x float> %coalesced_input, %coalesced_input1
; CHECK: %coalesced_output = shufflevector <16 x float> %coalesced_binary
; CHECK: %coalesced_output2 = shufflevector <16 x float> %coalesced_binary
; CHECK: %s1 = fsub <8 x float> %coalesced_output, %coalesced_output
; CHECK: %s2 = fsub <8 x float> %s1, %coalesced_output2
; CHECK-NOT: coalesced
; CHECK: ret void
define spir_kernel void @commuted(ptr addrspace(1) %out, <8 x float> %a, <8 x float> %b, <8 x float> %c) #0 {
entry:
  %m1 = fmul <8 x float> %a, %b
  %m2 = fmul <8 x float> %b, %a
  %m3 = fmul <8 x float> %a, %c
  %s1 = fsub <8 x float> %m1, %m2
  %s2 = fsub <8 x float> %s1, %m3
  store <8 x float> %s2, ptr addrspace(1) %out, align 32
  ret void
}

; non-commutative, not a copy
; CHECK-LABEL: @not_commutative
; CHECK: %coalesced_input = shufflevector <8 x float> %a, <8 x float> %b, <16 x i32>
; CHECK: %coalesced_input1 = shufflevector <8 x float> %b, <8 x float> %a, <16 x i32>
; CHECK: %coalesced_binary = fsub <16 x float> %coalesced_input, %coalesced_input1
; CHECK: %s = fmul <8 x float> %coalesced_output, %coalesced_output2
define spir_kernel void @not_commutative(ptr addrspace(1) %out, <8 x float> %a, <8 x float> %b) #0 {
entry:
  %d1 = fsub <8 x float> %a, %b
  %d2 = fsub <8 x float> %b, %a
  %s = fmul <8 x float> %d1, %d2
  store <8 x float> %s, ptr addrspace(1) %out, align 32
  ret void
}

; copy of a copy
; CHECK-LABEL: @chain
; CHECK: %m1 = fmul <8 x float> %a, %b
; CHECK-NOT: fmul
; CHECK: %coalesced_input = shufflevector <8 x float> %m1, <8 x float> %c, <16 x i32>
; CHECK: %coalesced_intrinsic = call <16 x float> @llvm.exp2.v16f32(<16 x float> %coalesced_input)
; CHECK: %s1 = fsub <8 x float> %coalesced_output, %coalesced_output
; CHECK: %s2 = fsub <8 x float> %s1, %coalesced_output1
; CHECK-NOT: coalesced
; CHECK: ret void
define spir_kernel void @chain(ptr addrspace(1) %out, <8 x float> %a, <8 x float> %b, <8 x float> %c) #0 {
entry:
  %m1 = fmul <8 x float> %a, %b
  %m2 = fmul <8 x float> %a, %b
  %e1 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %m1)
  %e2 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %m2)
  %e3 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %c)
  %s1 = fsub <8 x float> %e1, %e2
  %s2 = fsub <8 x float> %s1, %e3
  store <8 x float> %s2, ptr addrspace(1) %out, align 32
  ret void
}

; survivor keeps common flags
; CHECK-LABEL: @flags
; CHECK: %f1 = fadd nnan <8 x float> %a, %b
; CHECK: %e1 = call nnan ninf <8 x float> @llvm.exp2.v8f32(<8 x float> %f1)
; CHECK: %s = fsub <8 x float> %e1, %e1
define spir_kernel void @flags(ptr addrspace(1) %out, <8 x float> %a, <8 x float> %b) #0 {
entry:
  %f1 = fadd fast <8 x float> %a, %b
  %f2 = fadd nnan <8 x float> %b, %a
  %e1 = call fast <8 x float> @llvm.exp2.v8f32(<8 x float> %f1)
  %e2 = call nnan ninf <8 x float> @llvm.exp2.v8f32(<8 x float> %f2)
  %s = fsub <8 x float> %e1, %e2
  store <8 x float> %s, ptr addrspace(1) %out, align 32
  ret void
}

declare <8 x float> @llvm.exp2.v8f32(<8 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !10, !20, !30, !40}

!0 = !{ptr @exact, !1}
!10 = !{ptr @commuted, !1}
!20 = !{ptr @not_commutative, !1}
!30 = !{ptr @chain, !1}
!40 = !{ptr @flags, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @exact}
!14 = !{!"FuncMDValue[1]", !3}
!15 = !{!"FuncMDMap[1]", ptr @commuted}
!24 = !{!"FuncMDValue[2]", !3}
!25 = !{!"FuncMDMap[2]", ptr @not_commutative}
!34 = !{!"FuncMDValue[3]", !3}
!35 = !{!"FuncMDMap[3]", ptr @chain}
!44 = !{!"FuncMDValue[4]", !3}
!45 = !{!"FuncMDMap[4]", ptr @flags}
!6 = !{!"FuncMD", !5, !4, !15, !14, !25, !24, !35, !34, !45, !44}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
