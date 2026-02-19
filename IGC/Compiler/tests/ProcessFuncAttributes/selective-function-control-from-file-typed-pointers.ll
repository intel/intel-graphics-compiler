;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: split-file %s %t
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=3,SelectiveFunctionControl=1,PrintStackCallDebugInfo=1,SelectiveFunctionControlFile=%t/sfc1.ll < %t/test.ll | FileCheck %s --check-prefixes=CHECK-SFC1
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=3,SelectiveFunctionControl=1,PrintStackCallDebugInfo=1,SelectiveFunctionControlFile=%t/sfc2.ll < %t/test.ll | FileCheck %s --check-prefixes=CHECK-SFC2
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=3,SelectiveFunctionControl=1,PrintStackCallDebugInfo=1,SelectiveFunctionControlFile=%t/sfc3.ll < %t/test.ll | FileCheck %s --check-prefixes=CHECK-SFC3
; ------------------------------------------------

; Test checks function control setting from custom file

;--- sfc1.ll
FLAG_FCALL_DEFAULT:
foo1
foo2
foo3
declared_foo

;--- sfc2.ll
FLAG_FCALL_FORCE_INLINE:
foo1
FLAG_FCALL_FORCE_SUBROUTINE:
foo2
FLAG_FCALL_FORCE_STACKCALL:
foo3
FLAG_FCALL_FORCE_INDIRECTCALL:
declared_foo

;--- sfc3.ll
foo1
FLAG_FCALL_FORCE_INLINE:
FLAG_FCALL_FORCE_SUBROUTINE:
foo2
FLAG_FCALL_FORCE_STACKCALL:
foo3
//FLAG_FCALL_FORCE_INDIRECTCALL: comment is ignored
declared_foo

;--- test.ll
define spir_func i32 @foo1(i32* %src) #0 {
; CHECK-SFC1-LABEL: define internal spir_func i32 @foo1(
; CHECK-SFC1-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
; CHECK-SFC2-LABEL: define internal spir_func i32 @foo1(
; CHECK-SFC2-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
; CHECK-SFC3-LABEL: define internal spir_func i32 @foo1(
; CHECK-SFC3-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
  %1 = load i32, i32* %src
  %2 = add i32 %1, 144
  ret i32 %2
}

define spir_func i32 @foo2(i32* %src) #1 {
; CHECK-SFC1-LABEL: define internal spir_func i32 @foo2(
; CHECK-SFC1-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
; CHECK-SFC2-LABEL: define internal spir_func i32 @foo2(
; CHECK-SFC2-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
; CHECK-SFC3-LABEL: define internal spir_func i32 @foo2(
; CHECK-SFC3-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
  %1 = load i32, i32* %src
  %2 = mul i32 %1, 13
  ret i32 %2
}

define spir_func i32 @foo3(i32* %src) #0 {
; CHECK-SFC1-LABEL: define internal spir_func i32 @foo3(
; CHECK-SFC1-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
; CHECK-SFC2-LABEL: define internal spir_func i32 @foo3(
; CHECK-SFC2-SAME: i32* [[SRC:%.*]]) #[[ATTR2:[0-9]+]] {
;
; CHECK-SFC3-LABEL: define internal spir_func i32 @foo3(
; CHECK-SFC3-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
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
; CHECK-SFC1-LABEL: declare spir_func void @declared_foo(
; CHECK-SFC1-SAME: i32) #[[ATTR2:[0-9]+]]

; CHECK-SFC2-LABEL: declare spir_func void @declared_foo(
; CHECK-SFC2-SAME: i32) #[[ATTR3:[0-9]+]]

; CHECK-SFC3-LABEL: declare spir_func void @declared_foo(
; CHECK-SFC3-SAME: i32) #[[ATTR2:[0-9]+]]

attributes #0 = { noinline optnone }
attributes #1 = { alwaysinline }

!igc.functions = !{!1}
!1 = !{void (i32*)* @test_kernel, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}

; CHECK-SFC1: attributes #[[ATTR0]] = { noinline optnone "visaStackCall" }
; CHECK-SFC1: attributes #[[ATTR1]] = { noinline "visaStackCall" }
; CHECK-SFC1: attributes #[[ATTR2]] = { "referenced-indirectly" "visaStackCall" }
;.
; CHECK-SFC2: attributes #[[ATTR0]] = { alwaysinline }
; CHECK-SFC2: attributes #[[ATTR1]] = { noinline }
; CHECK-SFC2: attributes #[[ATTR2]] = { noinline optnone "visaStackCall" }
; CHECK-SFC2: attributes #[[ATTR3]] = { noinline "referenced-indirectly" "visaStackCall" }
;.
; CHECK-SFC3: attributes #[[ATTR0]] = { noinline optnone "visaStackCall" }
; CHECK-SFC3: attributes #[[ATTR1]] = { noinline }
; CHECK-SFC3: attributes #[[ATTR2]] = { noinline "visaStackCall" }
