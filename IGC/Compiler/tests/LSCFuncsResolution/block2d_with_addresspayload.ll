;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-lsc-funcs-translation -platformpvc -S  %s | FileCheck %s

; Check lowering of LSC 2D block messages with address payload

define spir_kernel void @test_lsc2DBlock_addressPayload(i64 %base, i64 %out, i32 %x, i32 %y, i32 %k) {
; CHECK-LABEL: @test_lsc2DBlock_addressPayload

; CHECK:  [[AP0:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1)
  %AP = call spir_func ptr @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1);

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[AP0]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @__builtin_IB_subgroup_block_read_ap_prefetch_u16_m8k16v1(ptr %AP, i32 0, i32 0, i32 0)

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[AP0]], i32 5, i32 %k, i1 false)
  call spir_func void @__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(ptr %AP, i32 %k)

; CHECK: [[TMP0:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[AP0]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %V0 = call spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(ptr %AP, i32 0, i32 0, i32 0)

; CHECK:  [[AP1:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out, i32 1023, i32 2047, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1)
  %AP.out = call spir_func ptr @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %out, i32 1023, i32 2047, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1);

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i16(ptr [[AP1]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> [[TMP0]])
 call spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(ptr %AP.out, i32 0, i32 0, <8 x i16> %V0, i32 0)

; CHECK: [[AP2:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCopyAddrPayload.p0(ptr [[AP0]])
  %AP2 = call spir_func ptr @__builtin_IB_subgroup_copyBlock2DAddressPayload(ptr %AP)

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i64(ptr [[AP2]], i32 1, i64 %base1, i1 false)
  %base1 = add i64 %base, 1024
  call spir_func void @__builtin_IB_subgroup_setBlock2DAddressPayloadBase(ptr %AP2, i64 %base1)

; CHECK: [[TMP1:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[AP2]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %V1 = call spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(ptr %AP2, i32 0, i32 0, i32 0)

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[AP1]], i32 6, i32 1024, i1 true)
  call spir_func void @__builtin_IB_subgroup_addBlock2DAddressPayloadBlockY(ptr %AP.out, i32 1024)

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i16(ptr [[AP1]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> [[TMP1]])
 call spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(ptr %AP.out, i32 0, i32 0, <8 x i16> %V1, i32 0)

; CHECK:  [[AP3:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 8, i32 8, i32 1)
; CHECK:  [[tmp2:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[AP3]], i32 0, i32 0, i32 32, i32 8, i32 8, i32 1, i1 true, i1 false, i32 0)
; CHECK:  [[tmp4:%.*]] = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v4i16.p0(ptr [[AP3]], i32 32, i32 0, i32 16, i32 8, i32 8, i32 1, i1 false, i1 true, i32 0)
  %AP3 = call spir_func ptr @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 8, i32 8, i32 1);
  %V3 = call spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_transpose_u32_m8k8v1(ptr %AP3, i32 0, i32 0, i32 0)
  %V4 = call spir_func <4 x i16> @__builtin_IB_subgroup_block_read_ap_transform_u16_m8k8v1(ptr %AP3, i32 32, i32 0, i32 0)

; CHECK:    ret void
  ret void
}

declare spir_func ptr @__builtin_IB_subgroup_createBlock2DAddressPayload(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare spir_func ptr @__builtin_IB_subgroup_copyBlock2DAddressPayload(ptr)
declare spir_func void @__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(ptr, i32)
declare spir_func void @__builtin_IB_subgroup_addBlock2DAddressPayloadBlockY(ptr, i32)
declare spir_func void @__builtin_IB_subgroup_setBlock2DAddressPayloadBase(ptr, i64)
declare spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(ptr, i32, i32, i32)
declare spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(ptr, i32, i32, <8 x i16>, i32)
declare spir_func void @__builtin_IB_subgroup_block_read_ap_prefetch_u16_m8k16v1(ptr, i32, i32, i32)
declare spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_transpose_u32_m8k8v1(ptr, i32, i32, i32)
declare spir_func <4 x i16> @__builtin_IB_subgroup_block_read_ap_transform_u16_m8k8v1(ptr, i32, i32, i32)

!igc.functions = !{!0}

!0 = !{ptr @test_lsc2DBlock_addressPayload, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
