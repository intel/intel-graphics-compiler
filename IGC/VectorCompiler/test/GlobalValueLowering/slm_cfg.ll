;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; COM: This test verifies whether an SLM global variable uses are updated correctly
; COM: in the following cyclic graph:

;        kernelA             kernelB
;           /                  \
;          f0                  f3
;         / \\                 /
;        /   \\               /
;       f1   f2              /
;        \   /              /
;         foo              /
;          \              /
;           \            /
;            \          /
;             \        /
;              \      /
;                bar

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@SLM_GV = internal addrspace(3) global [4 x i32] undef, align 4 #0

; COM: @SLM_GV access in this node is not supported.
define internal spir_func i32 @bar(i32 addrspace(3)* %arg) #1 {
  %arg.ld = load i32, i32 addrspace(3)* %arg, align 4
  ret i32 %arg.ld
}

; CHECK: define internal spir_func i32 @foo
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 0 to [4 x i32] addrspace(3)*
; CHECK-NEXT: %gaddr.lowering = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 1
define internal spir_func i32 @foo(i32 addrspace(3)* %arg) #1 {
  %arg.ld = load i32, i32 addrspace(3)* %arg, align 4
  %bar.res = call spir_func i32 @bar(i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 1))
  %res = add i32 %bar.res, %arg.ld
  ret i32 %res
}

; CHECK: define internal spir_func i32 @f1
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 0 to [4 x i32] addrspace(3)*
; CHECK-NEXT: %gv.p3 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 2
define internal spir_func i32 @f1() #1 {
  %gv.p3 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 2
  %foo.res = call spir_func i32 @foo(i32 addrspace(3)* %gv.p3)
  ret i32 %foo.res
}

; CHECK: define internal spir_func i32 @f2
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 0 to [4 x i32] addrspace(3)*
define internal spir_func i32 @f2() #1 {
  ; CHECK: %gv.p3.0 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 0
  %gv.p3.0 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 0
  %gv.ld.0 = load i32, i32 addrspace(3)* %gv.p3.0, align 4

  ; CHECK: %gv.p3.3 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 3
  %gv.p3.3 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 3
  %gv.ld.3 = load i32, i32 addrspace(3)* %gv.p3.3, align 4

  %sum = add i32 %gv.ld.0, %gv.ld.3
  store i32 %sum, i32 addrspace(3)* %gv.p3.0

  %f0.res = call spir_func i32 @f0(i32 addrspace(3)* %gv.p3.0)
  %foo.res = call spir_func i32 @foo(i32 addrspace(3)* %gv.p3.3)
  %sum.res = add i32 %f0.res, %foo.res
  ret i32 %sum.res
}

; CHECK: define internal spir_func i32 @f0
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 0 to [4 x i32] addrspace(3)*
define internal spir_func i32 @f0(i32 addrspace(3)* %arg) #1 {
  %arg.ld = load i32, i32 addrspace(3)* %arg, align 4

  ; CHECK: %gv.p3 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 0
  %gv.p3 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 0
  %gv.ld = load i32, i32 addrspace(3)* %gv.p3, align 4

  %sum = add i32 %arg.ld, %gv.ld
  %c = icmp uge i32 %sum, 25
  br i1 %c, label %call_f1, label %call_f2
call_f1:
  %f1.res = call spir_func i32 @f1()
  br label %exit
call_f2:
  %f2.res = call spir_func i32 @f2()
  br label %exit
exit:
  %f0.res = phi i32 [ %f1.res, %call_f1 ], [ %f2.res, %call_f2 ]
  ret i32 %f0.res
}

; CHECK: define internal spir_func i32 @f3
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 64 to [4 x i32] addrspace(3)*
; CHECK-NEXT: %gaddr.lowering = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 0
; CHECK-NEXT: %bar.res = call spir_func i32 @bar(i32 addrspace(3)* %gaddr.lowering)
define internal spir_func i32 @f3(i32 addrspace(3)* %arg) #1 {
  %bar.res = call spir_func i32 @bar(i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 0))
  %arg.ld = load i32, i32 addrspace(3)* %arg, align 4
  %sum = add i32 %bar.res, %arg.ld
  ret i32 %sum
}

; CHECK: define dllexport spir_kernel void @kernelA
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 0 to [4 x i32] addrspace(3)*
; CHECK-NEXT: %gaddr.lowering = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 0
; CHECK-NEXT: %res = call spir_func i32 @f0(i32 addrspace(3)* %gaddr.lowering)
define dllexport spir_kernel void @kernelA() #2 {
  %res = call spir_func i32 @f0(i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 0))
  ret void
}

; CHECK: define dllexport spir_kernel void @kernelB
; CHECK-NEXT: %SLM_GV.lowered = inttoptr i32 64 to [4 x i32] addrspace(3)*
; CHECK-NEXT: %gaddr.lowering = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* %SLM_GV.lowered, i64 0, i64 3
; CHECK-NEXT: %res = call spir_func i32 @f3(i32 addrspace(3)* %gaddr.lowering)
define dllexport spir_kernel void @kernelB() #2 {
  %res = call spir_func i32 @f3(i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @SLM_GV, i64 0, i64 3))
  ret void
}

; CHECK: !{{[[:digit:]]}} = !{void ()* @kernelA, !"kernelA", !{{[[:digit:]]}}, i32 16, !{{[[:digit:]]}}, !{{[[:digit:]]}}, !{{[[:digit:]]}}, i32 0}
; CHECK: !{{[[:digit:]]}} = !{void ()* @kernelB, !"kernelB", !{{[[:digit:]]}}, i32 80, !{{[[:digit:]]}}, !{{[[:digit:]]}}, !{{[[:digit:]]}}, i32 0}

attributes #0 = { "VCGlobalVariable" }
attributes #1 = { noinline nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!0, !1}
!genx.kernel.internal = !{!4, !5}

; COM: Initial slm size for kernelA is 0
!0 = !{void ()* @kernelA, !"kernelA", !2, i32 0, !2, !2, !3, i32 0}
; COM: Initial slm size for kernelB is 64
!1 = !{void ()* @kernelB, !"kernelB", !2, i32 64, !2, !2, !3, i32 0}
!2 = !{}
!3 = !{!""}
!4 = !{void ()* @kernelA, !2, !2, !2, !2}
!5 = !{void ()* @kernelB, !2, !2, !2, !2}
