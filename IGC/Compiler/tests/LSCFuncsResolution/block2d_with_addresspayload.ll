;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-lsc-funcs-translation -platformpvc -S  %s | FileCheck %s

; Check lowering of LSC 2D block messages with address payload

define spir_kernel void @test_lsc2DBlock_addressPayload(i64 %base, i64 %out, i32 %x, i32 %y, i32 %k) {
; CHECK-LABEL: @test_lsc2DBlock_addressPayload

; CHECK:  [[TMP0:%.*]] = call i32 @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1)
  %AP = call spir_func i32 @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1);

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.isVoid(i32 [[TMP0]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @__builtin_IB_subgroup_block_read_ap_prefetch_u16_m8k16v1(i32 %AP, i32 0, i32 0, i32 0)

; CHECK: [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.i32(i32 [[TMP0]], i32 5, i32 %k, i1 false)
  %AP.0 = call spir_func i32 @__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(i32 %AP, i32 %k)

; CHECK: [[TMP2:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16(i32 [[TMP1]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %V0 = call spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(i32 %AP.0, i32 0, i32 0, i32 0)

; CHECK:  [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload(i64 %out, i32 1023, i32 2047, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1)
  %AP.out = call spir_func i32 @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %out, i32 1023, i32 2047, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1);

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.v8i16(i32 [[TMP3]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> [[TMP2]])
 call spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(i32 %AP.out, i32 0, i32 0, <8 x i16> %V0, i32 0)

; CHECK: [[TMP4:%.*]] = call i32 @llvm.genx.GenISA.LSC2DBlockCopyAddrPayload(i32 [[TMP1]])
  %AP2 = call spir_func i32 @__builtin_IB_subgroup_copyBlock2DAddressPayload(i32 %AP.0)

; CHECK: [[TMP5:%.*]] = call i32 @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.i64(i32 [[TMP4]], i32 1, i64 %base1, i1 false)
  %base1 = add i64 %base, 1024
  %AP2.0 = call spir_func i32 @__builtin_IB_subgroup_setBlock2DAddressPayloadBase(i32 %AP2, i64 %base1)

; CHECK: [[TMP6:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16(i32 [[TMP5]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %V1 = call spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(i32 %AP2.0, i32 0, i32 0, i32 0)

; CHECK: [[TMP7:%.*]] = call i32 @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.i32(i32 [[TMP3]], i32 6, i32 1024, i1 true)
  %AP.out1 = call spir_func i32 @__builtin_IB_subgroup_addBlock2DAddressPayloadBlockY(i32 %AP.out, i32 1024)

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.v8i16(i32 [[TMP7]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> [[TMP6]])
 call spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(i32 %AP.out1, i32 0, i32 0, <8 x i16> %V1, i32 0)

; CHECK:    ret void
  ret void
}

declare spir_func i32 @__builtin_IB_subgroup_createBlock2DAddressPayload(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare spir_func i32 @__builtin_IB_subgroup_copyBlock2DAddressPayload(i32)
declare spir_func i32 @__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(i32, i32)
declare spir_func i32 @__builtin_IB_subgroup_addBlock2DAddressPayloadBlockY(i32, i32)
declare spir_func i32 @__builtin_IB_subgroup_setBlock2DAddressPayloadBase(i32, i64)
declare spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(i32, i32, i32, i32)
declare spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(i32, i32, i32, <8 x i16>, i32)
declare spir_func void @__builtin_IB_subgroup_block_read_ap_prefetch_u16_m8k16v1(i32, i32, i32, i32)

!igc.functions = !{!0}

!0 = !{void (i64, i64, i32, i32, i32)* @test_lsc2DBlock_addressPayload, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
