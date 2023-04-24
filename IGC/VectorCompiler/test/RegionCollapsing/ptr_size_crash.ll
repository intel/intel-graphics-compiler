;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: the test is actually a reduced example from dpcpp. the issue was
; COM: that we had a division by zero during an attempt to collapse a region
; COM: that have a pointers as an element type. This test is essentially checks
; COM: that we don't crash/trigger an assert
; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

; CHECK: pluto
; CHECK-NEXT: call spir_func void @foo(%0 addrspace(4)* null, i64 undef)
; CHECK-NEXT: ret void

source_filename = "/test.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64"

%0 = type { [1 x i64] }
%1 = type { %0 }

declare dso_local spir_func void @foo(%0 addrspace(4)*, i64) unnamed_addr #0
declare %1 addrspace(4)* @"llvm.genx.rdregioni.if-you-going-to-san-francisco::id.i16"(<1 x %1 addrspace(4)*>, i32, i32, i32, i16, i32) #0

define internal spir_func void @pluto(%1 addrspace(4)* %arg) unnamed_addr #0 {
  %tmp = inttoptr <1 x i64> zeroinitializer to <1 x %1 addrspace(4)*>
  %tmp1 = call %1 addrspace(4)* @"llvm.genx.rdregioni.if-you-going-to-san-francisco::id.i16"(<1 x %1 addrspace(4)*> %tmp, i32 0, i32 1, i32 1, i16 0, i32 0)
  %tmp2 = bitcast %1 addrspace(4)* %tmp1 to %0 addrspace(4)*
  call spir_func void @foo(%0 addrspace(4)* %tmp2, i64 undef) #1
  ret void
}

attributes #0 = { "target-cpu"="Gen9" }
attributes #1 = { noinline nounwind optnone }

