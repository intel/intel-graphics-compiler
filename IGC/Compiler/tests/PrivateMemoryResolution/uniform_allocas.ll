;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt -vulkan --inputps --opaque-pointers --regkey SupportUniformPrivateMemorySpace=0 --igc-private-mem-resolution --platformPtl -S %s 2>&1 | FileCheck %s --check-prefixes=CHECK,STATELESS_A64
; RUN: igc_opt -vulkan --inputps --opaque-pointers --regkey SupportUniformPrivateMemorySpace=1 --igc-private-mem-resolution --platformPtl -S %s 2>&1 | FileCheck %s --check-prefixes=CHECK,PRIVATE_A32,T_PRIVATE_A32
; RUN: igc_opt -vulkan --inputps --opaque-pointers --regkey SupportUniformPrivateMemorySpace=1  --regkey DisableSOAPromotion=1 --igc-private-mem-resolution --platformPtl -S %s 2>&1 | FileCheck %s --check-prefixes=CHECK,PRIVATE_A32,NT_PRIVATE_A32

define void @entry(i32 %idx) {
entry:
; Prolog - basic subgroup values
; CHECK:      [[LANE_ID_16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK-NEXT: [[LANE_ID:%.*]] = zext i16 [[LANE_ID_16]] to i32
; CHECK-NEXT: [[SIMD_SIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()

; Stateless base address
; STATELESS_A64-NEXT: [[STATELESS_BASE_ADDR_V2i32:%.*]] = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 0)
; STATELESS_A64-NEXT: [[STATELESS_BASE_ADDR:%.*]] = bitcast <2 x i32> [[STATELESS_BASE_ADDR_V2i32]] to i64

; per-thread address offset
; STATELESS_A64-NEXT: [[HWTID_i32:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; STATELESS_A64-NEXT: [[TOTAL_STRIDE_i32:%.*]] = mul i32 [[SIMD_SIZE]], 32768
; STATELESS_A64-NEXT: [[TOTAL_STRIDE:%.*]] = zext i32 [[TOTAL_STRIDE_i32]] to i64
; STATELESS_A64-NEXT: [[HWTID:%.*]] = zext i32 [[HWTID_i32]] to i64
; STATELESS_A64-NEXT: [[PERT_THREAD_OFFSET:%.*]] = mul i64 [[HWTID]], [[TOTAL_STRIDE]]
; STATELESS_A64-NEXT: [[STATELESS_THREAD_ADDR:%.*]] = add {{.*}} i64 [[STATELESS_BASE_ADDR]], [[PERT_THREAD_OFFSET]]

  %a = alloca [384 x <3 x float>], align 16, !uniform !0
; STATELESS_A64-NEXT: [[A_BUFFER_OFFSET_i32:%.*]] = mul i32 [[SIMD_SIZE]], 0
; STATELESS_A64-NEXT: [[A_BUFFER_OFFSET:%.*]] = zext i32 [[A_BUFFER_OFFSET_i32]] to i64
; STATELESS_A64-NEXT: [[A_BUFFER_ADDR_i64:%.*]] = add {{.*}} i64 [[STATELESS_THREAD_ADDR]], [[A_BUFFER_OFFSET]]
; STATELESS_A64-NEXT: [[A_BUFFER_ADDR:%.*]] = inttoptr i64 [[A_BUFFER_ADDR_i64]] to ptr addrspace(1)

  store [384 x <3 x float>] zeroinitializer, ptr %a
; STATELESS_A64-NEXT: store [384 x <3 x float>] zeroinitializer, ptr addrspace(1) [[A_BUFFER_ADDR]], align 16

; PRIVATE_A32-NEXT: store [384 x <3 x float>] zeroinitializer, ptr null, align 16

  %b = alloca [384 x <3 x float>], align 16, !uniform !0
; STATELESS_A64-NEXT: [[B_BUFFER_OFFSET_i32:%.*]] = mul i32 [[SIMD_SIZE]], 6144
; STATELESS_A64-NEXT: [[B_BUFFER_OFFSET:%.*]] = zext i32 [[B_BUFFER_OFFSET_i32]] to i64
; STATELESS_A64-NEXT: [[B_BUFFER_ADDR_i64:%.*]] = add {{.*}} i64 [[STATELESS_THREAD_ADDR]], [[B_BUFFER_OFFSET]]
; STATELESS_A64-NEXT: [[B_BUFFER_ADDR:%.*]] = inttoptr i64 [[B_BUFFER_ADDR_i64]] to ptr addrspace(1)

  store [384 x <3 x float>] zeroinitializer, ptr %b
; STATELESS_A64-NEXT: store [384 x <3 x float>] zeroinitializer, ptr addrspace(1) [[B_BUFFER_ADDR]], align 16

; PRIVATE_A32-NEXT: store [384 x <3 x float>] zeroinitializer, ptr inttoptr (i32 6144 to ptr), align 16

  %c = alloca [384 x <3 x float>], align 16
; STATELESS_A64-NEXT: [[C_BUFFER_OFFSET:%.*]] = mul i32 [[SIMD_SIZE]], 12288
; STATELESS_A64-NEXT: [[C_PER_LANE_OFFSET:%.*]] = mul i32 [[LANE_ID]], 6144
; STATELESS_A64-NEXT: [[C_SUBGROUP_BUFFER_OFFSET_i32:%.*]] = add i32 [[C_BUFFER_OFFSET]], [[C_PER_LANE_OFFSET:%.*]]
; STATELESS_A64-NEXT: [[C_SUBGROUP_BUFFER_OFFSET:%.*]] = zext i32 [[C_SUBGROUP_BUFFER_OFFSET_i32]] to i64
; STATELESS_A64-NEXT: [[C_BUFFER_ADDR_i64:%.*]] = add {{.*}} i64 [[STATELESS_THREAD_ADDR]], [[C_SUBGROUP_BUFFER_OFFSET]]
; STATELESS_A64-NEXT: [[C_BUFFER_ADDR:%.*]] = inttoptr i64 [[C_BUFFER_ADDR_i64]] to ptr addrspace(1)

; PRIVATE_A32-NEXT:    [[C_BUFFER_OFFSET:%.*]] = mul i32 [[SIMD_SIZE]], 0
; NT_PRIVATE_A32-NEXT: [[C_PER_LANE_OFFSET:%.*]] = mul i32 [[LANE_ID]], 6144
; T_PRIVATE_A32-NEXT:  [[C_PER_LANE_OFFSET:%.*]] = mul i32 [[LANE_ID]], 16
; PRIVATE_A32-NEXT:    [[C_SUBGROUP_BUFFER_OFFSET:%.*]] = add i32 [[C_BUFFER_OFFSET]], [[C_PER_LANE_OFFSET:%.*]]
; PRIVATE_A32-NEXT:    [[C_BUFFER_ADDR_i32:%.*]] = add {{.*}} i32 0, [[C_SUBGROUP_BUFFER_OFFSET]]
; PRIVATE_A32-NEXT:    [[C_BUFFER_ADDR:%.*]] = inttoptr i32 [[C_BUFFER_ADDR_i32]] to ptr

  %c.off = getelementptr [384 x <3 x float>], ptr %c, i32 0, i32 %idx
; STATELESS_A64-NEXT: [[C_ELEMENT_ADDR:%.*]] =  getelementptr [384 x <3 x float>], ptr addrspace(1) [[C_BUFFER_ADDR]], i32 0, i32 %idx

; NT_PRIVATE_A32-NEXT: [[C_ELEMENT_ADDR:%.*]] =  getelementptr [384 x <3 x float>], ptr [[C_BUFFER_ADDR]], i32 0, i32 %idx

; T_PRIVATE_A32-NEXT: [[TMP1:%.*]]  = add i32 0, %idx
; T_PRIVATE_A32-NEXT: [[TMP2:%.*]] = mul i32 [[TMP1]], 1
; T_PRIVATE_A32-NEXT: [[CHUNK_INDEX:%.*]] = add i32 [[TMP2]], 0
; T_PRIVATE_A32-NEXT: [[CHUNK_STRIDE:%.*]] = mul i32 [[SIMD_SIZE]], 16
; T_PRIVATE_A32-NEXT: [[CHUNK_OFFSET:%.*]] = mul i32 [[CHUNK_INDEX]], [[CHUNK_STRIDE]]
; T_PRIVATE_A32-NEXT: [[C_ELEMENT_ADDR_I32:%.*]] = add i32 [[C_BUFFER_ADDR_i32]], [[CHUNK_OFFSET]]
; T_PRIVATE_A32-NEXT: [[C_ELEMENT_ADDR:%.*]] = inttoptr i32 [[C_ELEMENT_ADDR_I32]] to ptr

  store <3 x float> zeroinitializer, ptr %c.off
; STATELESS_A64-NEXT: store <3 x float> zeroinitializer, ptr addrspace(1) [[C_ELEMENT_ADDR]], align 16

; PRIVATE_A32-NEXT: store <3 x float> zeroinitializer, ptr [[C_ELEMENT_ADDR]], align 16

  ret void
; CHECK-NEXT: ret void
}

!igc.functions = !{!1}

!0 = !{i1 true}
!1 = !{void ()* @entry, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
