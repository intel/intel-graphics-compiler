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
; __kernel void test_bar(__global uint* dst, int src)
; {
;    sub_group_barrier(CLK_LOCAL_MEM_FENCE);
;    dst[0] = src;
; }
;
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_bar(i32 addrspace(1)* %dst, i32 %src) #0 {
; CHECK-LABEL: @test_bar(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    store i32 [[SRC:%.*]], i32* [[SRC_ADDR]], align 4
; CHECK:    call void @llvm.genx.GenISA.wavebarrier()
; CHECK:    [[TMP0:%.*]] = load i32, i32* [[SRC_ADDR]], align 4
; CHECK:    [[TMP1:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP1]], i64 0
; CHECK:    store i32 [[TMP0]], i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 %src, i32* %src.addr, align 4
  call spir_func void @__builtin_IB_sub_group_barrier()
  %0 = load i32, i32* %src.addr, align 4
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func void @__builtin_IB_sub_group_barrier() local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }

!IGCMetadata = !{!3}
!igc.functions = !{}

!3 = !{!"ModuleMD", !4}
!4 = !{!"compOpt", !5}
!5 = !{!"OptDisable", i1 false}
