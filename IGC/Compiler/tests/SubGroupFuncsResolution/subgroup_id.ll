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
; __kernel void test_sub(global int *dst)
; {
;     int sub_id = get_sub_group_local_id();
;     dst[0] = sub_id;
; }
;
; ------------------------------------------------
;
; get_sub_group_local_id transformation

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_sub(i32 addrspace(1)* %dst) #0 {
; CHECK-LABEL: @test_sub(
; CHECK-NEXT:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SUB_ID:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[SIMDLANEID16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:    [[SIMDLANEID:%.*]] = zext i16 [[SIMDLANEID16]] to i32
; CHECK:    store i32 [[SIMDLANEID]], i32* [[SUB_ID]], align 4
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %sub_id = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  %call.i = call spir_func i32 @__builtin_IB_get_simd_id()
  store i32 %call.i, i32* %sub_id, align 4
  %0 = load i32, i32* %sub_id, align 4
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i32 @__builtin_IB_get_simd_id()

