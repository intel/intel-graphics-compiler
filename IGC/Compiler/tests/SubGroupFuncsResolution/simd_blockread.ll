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
; This test checks that SubGroupFuncsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_bread(__global uint* dst, __global uint* src)
; {
;    int b_read = intel_sub_group_block_read(src);
;    dst[0] = b_read;
; }
;
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_bread(i32 addrspace(1)* %dst, i32 addrspace(1)* %src) #0 {
; CHECK-LABEL: @test_bread(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[B_READ:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    store i32 addrspace(1)* [[SRC:%.*]], i32 addrspace(1)** [[SRC_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRC_ADDR]], align 8
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)* [[TMP0]])
; CHECK:    store i32 [[TMP1]], i32* [[B_READ]], align 4
;
; CHECK:  declare i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)*) [[ATTR:#.*]]
; CHECK:  attributes [[ATTR]] = { {{.*convergent.*}} }
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32 addrspace(1)*, align 8
  %b_read = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 addrspace(1)* %src, i32 addrspace(1)** %src.addr, align 8
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8
  %call.i = call spir_func i32 @__builtin_IB_simd_block_read_1_global(i32 addrspace(1)* %0)
  store i32 %call.i, i32* %b_read, align 4
  %1 = load i32, i32* %b_read, align 4
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 0
  store i32 %1, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i32 @__builtin_IB_simd_block_read_1_global(i32 addrspace(1)*) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}

!3 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @test_bread, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"sub_group_size", i32 16}
