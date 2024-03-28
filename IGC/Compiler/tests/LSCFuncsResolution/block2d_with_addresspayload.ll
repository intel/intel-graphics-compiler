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

; CHECK:  [[TMP0:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1)
  %AP = call spir_func <8 x i32> @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %base, i32 1023, i32 1023, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1);

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.isVoid(<8 x i32> [[TMP0]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call spir_func void @__builtin_IB_subgroup_block_read_ap_prefetch_u16_m8k16v1(<8 x i32> %AP, i32 0, i32 0, i32 0)

; CHECK: [[TMP01:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockSetBlockXY(<8 x i32> [[TMP0]], i32 %k, i1 false, i1 true)
  %AP.0 = call spir_func <8 x i32> @__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(<8 x i32> %AP, i32 %k)

; CHECK: [[TMP2:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16(<8 x i32> [[TMP01]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %V = call spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(<8 x i32> %AP.0, i32 0, i32 0, i32 0)


; CHECK:  [[TMP1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload(i64 %out, i32 1023, i32 2047, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1)
  %AP.out = call spir_func <8 x i32> @__builtin_IB_subgroup_createBlock2DAddressPayload(i64 %out, i32 1023, i32 2047, i32 1023, i32 %x, i32 %y, i32 16, i32 8, i32 1);

; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.v8i16(<8 x i32> [[TMP1]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> [[TMP2]])
 call spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(<8 x i32> %AP.out, i32 0, i32 0, <8 x i16> %V, i32 0)

; CHECK:    ret void
  ret void
}

declare spir_func <8 x i32> @__builtin_IB_subgroup_createBlock2DAddressPayload(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare spir_func <8 x i32> @__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(<8 x i32>, i32)
declare spir_func <8 x i16> @__builtin_IB_subgroup_block_read_ap_u16_m8k16v1(<8 x i32>, i32, i32, i32)
declare spir_func void @__builtin_IB_subgroup_block_write_ap_u16_m8k16v1(<8 x i32>, i32, i32, <8 x i16>, i32)
declare spir_func void @__builtin_IB_subgroup_block_read_ap_prefetch_u16_m8k16v1(<8 x i32>, i32, i32, i32)

!igc.functions = !{!0}

!0 = !{void (i64, i64, i32, i32, i32)* @test_lsc2DBlock_addressPayload, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
