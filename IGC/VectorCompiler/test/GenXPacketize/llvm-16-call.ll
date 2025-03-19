;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
;
; RUN: %opt_new_pm_typed -passes=GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_new_pm_opaque -passes=GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
;
; ------------------------------------------------
; GenXPacketize
; ------------------------------------------------

; CHECK: call{{.*}}mandel_loop{{.*}} #[[ATTRIB:[0-9]*]]
; CHECK-DAG: attributes #[[ATTRIB]] = { alwaysinline nounwind }

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

define internal spir_func void @_Z11mandel_loopu2CMvr16_iS_S_ifff(<16 x i32>* %m, <16 x i32>* %ix, <16 x i32>* %iy, i32 %crunch, float %xOff, float %yOff, float %scale) #0 {
entry:
; COM: Various instruction to check that it doesn't crash
  %0 = alloca i32, align 4
  %1 = addrspacecast i32* %0 to i32 addrspace(4)*
  %2 = load i32, i32 addrspace(4)* %1, align 4
  %3 = tail call i32 @llvm.ctlz.i32(i32 %2, i1 false)
  ret void
}

define spir_kernel void @mandelbrot() {
entry:
  br label %if.end

if.then:                                          ; No predecessors!
  call spir_func void @_Z11mandel_loopu2CMvr16_iS_S_ifff(<16 x i32>* null, <16 x i32>* null, <16 x i32>* null, i32 0, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00) #1
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

declare i32 @llvm.ctlz.i32(i32, i1 immarg)

attributes #0 = { "CMGenxSIMT"="16" }
attributes #1 = { noinline nounwind }
