;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-sub-group-func-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; Was reduced from ocl test kernel:
;
;
; __kernel void test_reduce(__global uint* dst, int src)
; {
;    int re = sub_group_reduce_add(src);
;    dst[0] = re;
; }
;
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_reduce(i32 addrspace(1)* %dst, i32 %src) #0 {
; CHECK-LABEL: @test_reduce(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca i32, align 4
; CHECK:    [[RE:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    store i32 [[SRC:%.*]], i32* [[SRC_ADDR]], align 4
; CHECK:    [[TMP0:%.*]] = load i32, i32* [[SRC_ADDR]], align 4
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 [[TMP0]], i8 0, i1 true, i32 0)
; CHECK:    store i32 [[TMP1]], i32* [[RE]], align 4
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  %re = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 %src, i32* %src.addr, align 4
  %0 = load i32, i32* %src.addr, align 4
  %call82.i = call spir_func i32 @__builtin_IB_sub_group_reduce_IAdd_i32(i32 %0)
  store i32 %call82.i, i32* %re, align 4
  %1 = load i32, i32* %re, align 4
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 0
  store i32 %1, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i32 @__builtin_IB_sub_group_reduce_IAdd_i32(i32) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}

!3 = !{void (i32 addrspace(1)*, i32)* @test_reduce, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"sub_group_size", i32 16}
