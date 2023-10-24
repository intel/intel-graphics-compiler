;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; CHECK-DAG: foo_kernelA
; CHECK-DAG: switchjmp (M1, 1)
; CHECK-DAG: foo_kernelB
; CHECK-DAG: switchjmp (M1, 1)
define internal spir_func void @foo(i32 addrspace(1)* %jmpcptr, i32 addrspace(1)* %out) #0 {
  %jmpc = load i32, i32 addrspace(1)* %jmpcptr, align 8
  switch i32 %jmpc, label %deflt [
    i32 0, label %exit
    i32 1, label %labA
    i32 2, label %labB
    i32 3, label %labC
    i32 4, label %labD
  ]
labA:
  store i32 1, i32 addrspace(1)* %out, align 4
  br label %exit
labB:
  store i32 11, i32 addrspace(1)* %out, align 4
  br label %exit
labC:
  store i32 111, i32 addrspace(1)* %out, align 4
  br label %exit
labD:
  store i32 1111, i32 addrspace(1)* %out, align 4
  br label %exit
deflt:
  store i32 11111, i32 addrspace(1)* %out, align 4
  br label %exit
exit:
  ret void
}

define dllexport spir_kernel void @kernelA(i32 addrspace(1)* %in) local_unnamed_addr #1 {
  call spir_func void @foo(i32 addrspace(1)* %in, i32 addrspace(1)* %in)
  ret void
}

define dllexport spir_kernel void @kernelB(i32 addrspace(1)* %in) local_unnamed_addr #1 {
  call spir_func void @foo(i32 addrspace(1)* %in, i32 addrspace(1)* %in)
  ret void
}

attributes #0 = { noinline }
attributes #1 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!genx.kernels = !{!4, !40}
!genx.kernel.internal = !{!8, !80}

!0 = !{i32 0}
!4 = !{void (i32 addrspace(1)*)* @kernelA, !"kernelA", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 112}
!6 = !{i32 64}
!7 = !{}
!8 = !{void (i32 addrspace(1)*)* @kernelA, null, null, null, null}

!10 = !{i32 1}
!40 = !{void (i32 addrspace(1)*)* @kernelB, !"kernelB", !50, i32 0, !60, !10, !70, i32 0}
!50 = !{i32 112}
!60 = !{i32 64}
!70 = !{}
!80 = !{void (i32 addrspace(1)*)* @kernelB, null, null, null, null}
