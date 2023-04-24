;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStackUsage -march=genx64 -stack-analysis -mcpu=Gen9 -S %s 2> %t.stderr-out
; RUN: FileCheck %s < %t.stderr-out
; CHECK: 1152
; CHECK: 1152
; CHECK: 720
; CHECK: 432
; CHECK: 408

source_filename = "main.c"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "x86_64-pc-linux-gnu"

%struct._b128 = type { i32, [3 x i32] }
%struct._b1024 = type { [4 x %struct._b128], [16 x float] }
%struct._b2048 = type { %struct._b1024, [16 x float], [2 x %struct._b128], [8 x i32] }

define internal spir_func void @f128() {
  %1 = alloca %struct._b128, align 4
  ret void
}

define internal spir_func void @f512() {
  %1 = alloca [4 x %struct._b128], align 16
  ret void
}

define internal spir_func void @f1024() {
  %1 = alloca %struct._b1024, align 4
  ret void
}

define internal spir_func void @f1536() {
  %1 = alloca %struct._b1024, align 4
  %2 = alloca [4 x %struct._b128], align 16
  ret void
}

define internal spir_func void @f3200() {
  %1 = alloca %struct._b128, align 4
  %2 = alloca %struct._b1024, align 4
  %3 = alloca %struct._b2048, align 4
  ret void
}

define internal spir_func void @sum() {
  %1 = alloca %struct._b128, align 4
  call spir_func void @f3200()
  call spir_func void @f128()
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define internal spir_func void @mul() {
  %1 = alloca %struct._b2048, align 32
  call spir_func void @f1024()
  call spir_func void @sum()
  call spir_func void @f1536()
  call spir_func void @f512()
  ret void
}

define internal spir_func void @kernel() {
  %1 = alloca %struct._b128, align 4
  %2 = alloca %struct._b1024, align 16
  %3 = alloca %struct._b2048, align 4
  %4 = alloca <2 x i64>
  call spir_func void @sum()
  call spir_func void @mul()
  call spir_func void @sum()
  ret void
}

define dllexport spir_kernel void @main() #0 {
  call spir_func void @kernel()
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernel.internal = !{!0}
!0 = !{void ()* @main}
