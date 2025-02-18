;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -regkey FunctionControl=3,SelectiveFunctionControl=2,SelectiveFunctionControlFile=%t -disable-output < %s
; RUN: cat %t | FileCheck %s
; ------------------------------------------------

; Test checks that file with callable functions is created:

; CHECK-DAG: foo1
; CHECK-DAG: foo2
; CHECK-DAG: foo3
; CHECK-NOT: declared_foo

define spir_func i32 @foo1(i32* %src) #0 {
  %1 = load i32, i32* %src
  %2 = add i32 %1, 144
  ret i32 %2
}

define spir_func i32 @foo2(i32* %src) #1 {
  %1 = load i32, i32* %src
  %2 = mul i32 %1, 13
  ret i32 %2
}

define spir_func i32 @foo3(i32* %src) #0 {
  %1 = load i32, i32* %src
  %2 = mul i32 %1, 13
  ret i32 %2
}

define spir_kernel void @test_kernel(i32* %a) #0 {
  %1 = call i32 @foo1(i32* %a)
  %2 = call i32 @foo2(i32* %a)
  %3 = call i32 @foo3(i32* %a)
  %4 = mul i32 %1, %2
  %5 = add i32 %4, %3
  call void @declared_foo(i32 %5)
  ret void
}

declare spir_func void @declared_foo(i32)

attributes #0 = { noinline optnone }
attributes #1 = { alwaysinline }

!igc.functions = !{!1}
!1 = !{void (i32*)* @test_kernel, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
