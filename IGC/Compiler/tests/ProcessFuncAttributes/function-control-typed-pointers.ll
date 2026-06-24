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
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-FC0
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=1 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-FC1
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=2 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-FC2
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=3 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-FC3
; RUN: igc_opt --typed-pointers --igc-process-func-attributes -S -regkey FunctionControl=4 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-FC4
; ------------------------------------------------

; Test check forced attribute update based on FunctionControl flag:
;   FLAG_FCALL_DEFAULT = 0,
;   FLAG_FCALL_FORCE_INLINE = 1,
;   FLAG_FCALL_FORCE_SUBROUTINE = 2,
;   FLAG_FCALL_FORCE_STACKCALL = 3,
;   FLAG_FCALL_FORCE_INDIRECTCALL = 4

define spir_func i32 @foo1(i32* %src) #0 {
; CHECK-FC0-LABEL: define internal spir_func i32 @foo1(
; CHECK-FC0-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
; CHECK-FC1-LABEL: define internal spir_func i32 @foo1(
; CHECK-FC1-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
; CHECK-FC2-LABEL: define internal spir_func i32 @foo1(
; CHECK-FC2-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
; CHECK-FC3-LABEL: define internal spir_func i32 @foo1(
; CHECK-FC3-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
; CHECK-FC4-LABEL: define dso_local spir_func i32 @foo1(
; CHECK-FC4-SAME: i32* [[SRC:%.*]]) #[[ATTR0:[0-9]+]] {
;
  %1 = load i32, i32* %src
  %2 = add i32 %1, 144
  ret i32 %2
}

define spir_func i32 @foo2(i32* %src) #1 {
; CHECK-FC0-LABEL: define internal spir_func i32 @foo2(
; CHECK-FC0-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
; CHECK-FC1-LABEL: define internal spir_func i32 @foo2(
; CHECK-FC1-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
; CHECK-FC2-LABEL: define internal spir_func i32 @foo2(
; CHECK-FC2-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
; CHECK-FC3-LABEL: define internal spir_func i32 @foo2(
; CHECK-FC3-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
; CHECK-FC4-LABEL: define dso_local spir_func i32 @foo2(
; CHECK-FC4-SAME: i32* [[SRC:%.*]]) #[[ATTR1:[0-9]+]] {
;
  %1 = load i32, i32* %src
  %2 = mul i32 %1, 13
  ret i32 %2
}

define spir_func i32 @foo3(i32* %src) #0 {
; CHECK-FC0-LABEL: define internal spir_func i32 @foo3(
; CHECK-FC0-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
; CHECK-FC1-LABEL: define internal spir_func i32 @foo3(
; CHECK-FC1-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
; CHECK-FC2-LABEL: define internal spir_func i32 @foo3(
; CHECK-FC2-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
; CHECK-FC3-LABEL: define internal spir_func i32 @foo3(
; CHECK-FC3-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
; CHECK-FC4-LABEL: define dso_local spir_func i32 @foo3(
; CHECK-FC4-SAME: i32* [[SRC:%.*]]) #[[ATTR0]] {
;
  %1 = load i32, i32* %src
  %2 = mul i32 %1, 13
  ret i32 %2
}

define spir_kernel void @test_kernel(i32* %a) #0 {
; CHECK-FC0-LABEL: define spir_kernel void @test_kernel(
; CHECK-FC0-SAME: i32* [[A:%.*]]) #[[ATTR0]] {
;
; CHECK-FC1-LABEL: define spir_kernel void @test_kernel(
; CHECK-FC1-SAME: i32* [[A:%.*]]) #[[ATTR1:[0-9]+]] {
;
; CHECK-FC2-LABEL: define spir_kernel void @test_kernel(
; CHECK-FC2-SAME: i32* [[A:%.*]]) #[[ATTR0]] {
;
; CHECK-FC3-LABEL: define spir_kernel void @test_kernel(
; CHECK-FC3-SAME: i32* [[A:%.*]]) #[[ATTR2:[0-9]+]] {
;
; CHECK-FC4-LABEL: define spir_kernel void @test_kernel(
; CHECK-FC4-SAME: i32* [[A:%.*]]) #[[ATTR2:[0-9]+]] {
;
  %1 = call i32 @foo1(i32* %a)
  %2 = call i32 @foo2(i32* %a)
  %3 = call i32 @foo3(i32* %a)
  %4 = mul i32 %1, %2
  %5 = add i32 %4, %3
  call void @declared_foo(i32 %5)
  ret void
}

; CHECK-FC0-LABEL: declare spir_func void @declared_foo(
; CHECK-FC0-SAME: i32) #[[ATTR2:[0-9]+]]

; CHECK-FC1-LABEL: declare spir_func void @declared_foo(
; CHECK-FC1-SAME: i32) #[[ATTR2:[0-9]+]]

; CHECK-FC2-LABEL: declare spir_func void @declared_foo(
; CHECK-FC2-SAME: i32) #[[ATTR2:[0-9]+]]

; CHECK-FC3-LABEL: declare spir_func void @declared_foo(
; CHECK-FC3-SAME: i32) #[[ATTR3:[0-9]+]]

; CHECK-FC4-LABEL: declare spir_func void @declared_foo(
; CHECK-FC4-SAME: i32) #[[ATTR3:[0-9]+]]

declare spir_func void @declared_foo(i32)

attributes #0 = { noinline optnone }
attributes #1 = { alwaysinline }

!igc.functions = !{!1}
!1 = !{void (i32*)* @test_kernel, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
;.
; CHECK-FC0: attributes #[[ATTR0]] = { noinline optnone }
; CHECK-FC0: attributes #[[ATTR1]] = { alwaysinline }
; CHECK-FC0: attributes #[[ATTR2]] = { "referenced-indirectly" "visaStackCall" }
;.
; CHECK-FC1: attributes #[[ATTR0]] = { alwaysinline }
; CHECK-FC1: attributes #[[ATTR1]] = { noinline optnone }
; CHECK-FC1: attributes #[[ATTR2]] = { "referenced-indirectly" "visaStackCall" }
;.
; CHECK-FC2: attributes #[[ATTR0]] = { noinline optnone }
; CHECK-FC2: attributes #[[ATTR1]] = { noinline }
; CHECK-FC2: attributes #[[ATTR2]] = { "referenced-indirectly" "visaStackCall" }
;.
; CHECK-FC3: attributes #[[ATTR0]] = { noinline optnone "visaStackCall" }
; CHECK-FC3: attributes #[[ATTR1]] = { noinline "visaStackCall" }
; CHECK-FC3: attributes #[[ATTR2]] = { noinline optnone }
; CHECK-FC3: attributes #[[ATTR3]] = { "referenced-indirectly" "visaStackCall" }
;.
; CHECK-FC4: attributes #[[ATTR0]] = { noinline optnone "referenced-indirectly" "visaStackCall" }
; CHECK-FC4: attributes #[[ATTR1]] = { noinline "referenced-indirectly" "visaStackCall" }
; CHECK-FC4: attributes #[[ATTR2]] = { noinline optnone }
; CHECK-FC4: attributes #[[ATTR3]] = { "referenced-indirectly" "visaStackCall" }
