;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -enable-debugify --igc-sub-group-func-resolution -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -enable-debugify --igc-sub-group-func-resolution -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; Was reduced from ocl test kernel:
;
; __kernel void test_bwrite(__global uint* dst, int src)
; {
;     intel_sub_group_block_write(dst, src );
; }
;
; ------------------------------------------------

; Debug related check:
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_bwrite(i32 addrspace(1)* %dst, i32 %src) #0 {
; CHECK-LABEL: @test_bwrite(
; CHECK:  entry:
; CHECK-TYPED:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK-OPAQUE:    [[DST_ADDR:%.*]] = alloca ptr addrspace(1), align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca i32, align 4
; CHECK-TYPED:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK-OPAQUE:    store ptr addrspace(1) [[DST:%.*]], ptr [[DST_ADDR]], align 8
; CHECK-TYPED:    store i32 [[SRC:%.*]], i32* [[SRC_ADDR]], align 4
; CHECK-OPAQUE:    store i32 [[SRC:%.*]], ptr [[SRC_ADDR]], align 4
; CHECK-TYPED:    [[TMP0:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK-OPAQUE:    [[TMP0:%.*]] = load ptr addrspace(1), ptr [[DST_ADDR]], align 8
; CHECK-TYPED:    [[TMP1:%.*]] = load i32, i32* [[SRC_ADDR]], align 4
; CHECK-OPAQUE:    [[TMP1:%.*]] = load i32, ptr [[SRC_ADDR]], align 4
; CHECK-TYPED:    call void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)* [[TMP0]], i32 [[TMP1]])
; CHECK-OPAQUE:    call void @llvm.genx.GenISA.simdBlockWrite.p1.i32(ptr addrspace(1) [[TMP0]], i32 [[TMP1]])
;
; CHECK-TYPED:  declare void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)*, i32) [[ATTR:#.*]]
; CHECK-OPAQUE:  declare void @llvm.genx.GenISA.simdBlockWrite.p1.i32(ptr addrspace(1), i32) [[ATTR:#.*]]
; CHECK:  attributes [[ATTR]] = { {{.*convergent.*}} }
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 %src, i32* %src.addr, align 4
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %1 = load i32, i32* %src.addr, align 4
  call spir_func void @__builtin_IB_simd_block_write_1_global(i32 addrspace(1)* %0, i32 %1)
  ret void
}


declare spir_func void @__builtin_IB_simd_block_write_1_global(i32 addrspace(1)*, i32) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}

!3 = !{void (i32 addrspace(1)*, i32)* @test_bwrite, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
