;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-predefined-constant-resolve -check-debugify 2>&1 -S < %s | FileCheck %s
; ------------------------------------------------
; PredefinedConstantResolving
; ------------------------------------------------
;
; Was reduced from ocl test kernel:
;
; __constant int ga = 42;
;
; __kernel void test_const(__global uint* dst)
; {
;   int la = ga;
;   dst[0] = la;
; }
;
; ------------------------------------------------

; Debug-info related check
;
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 3
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

@ga = addrspace(2) constant i32 42, align 4

define spir_kernel void @test_const(i32 addrspace(1)* %dst) #0 {
; CHECK-LABEL: @test_const(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP0]], i64 0
; CHECK:    store i32 42, i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  %0 = load i32, i32 addrspace(2)* @ga, align 4
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4
  ret void
}


attributes #0 = { convergent noinline nounwind }
