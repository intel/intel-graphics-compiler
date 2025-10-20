;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; Check the case when DPAS instructions are in different basic blocks.

; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceRPOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --igc-code-scheduling --verify \
; RUN:         -S %s | FileCheck %s


define spir_kernel void @dpas_in_diff_bb(ptr addrspace(1) %_arg_A, ptr addrspace(1) %_arg_B, i16 %localIdY) {
; CHECK-LABEL: @dpas_in_diff_bb(

; CHECK:       for.body19.i:

; These 2 loads are used in different basic blocks but have the same MW and RP.
; %Block2D_ReadAddrPayload3 should be scheduled first because it's used in the same BB

; CHECK:         [[BLOCK2D_READADDRPAYLOAD3:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B:%.*]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         [[DPAS:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[BLOCK2D_READADDRPAYLOAD3]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD1:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD2:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

; CHECK:       dpas_bb:
; CHECK:         [[DPAS_1_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD1:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)

__igcbuiltin_u64_udiv_dp.exit:
  %mul56.i = shl i32 0, 8
  %0 = zext i16 %localIdY to i32
  %mul57.i = shl nuw nsw i32 %0, 5
  %add58.i = add i32 %mul56.i, %mul57.i
  %mul83.i = shl nuw nsw i64 0, 7
  %1 = shl nuw nsw i64 0, 9
  %2 = add nuw nsw i64 %1, %mul83.i
  %.ascast.i67 = addrspacecast ptr addrspace(1) %_arg_A to ptr addrspace(4)
  %3 = ptrtoint ptr addrspace(4) %.ascast.i67 to i64
  %.ascast.i68 = addrspacecast ptr addrspace(1) %_arg_A to ptr addrspace(4)
  %4 = ptrtoint ptr addrspace(4) %.ascast.i68 to i64
  %conv.i9.1 = or i32 %add58.i, 8
  %conv.i9.2 = or i32 %add58.i, 16
  %conv.i9.3 = or i32 %add58.i, 24
  %conv2.i = trunc i64 %2 to i32
  %qot = ashr exact i32 %conv2.i, 1
  %conv2.i.1 = or i32 %conv2.i, 32
  %qot781 = ashr exact i32 %conv2.i.1, 1
  %conv2.i.2 = or i32 %conv2.i, 64
  %qot787 = ashr exact i32 %conv2.i.2, 1
  %conv2.i.3 = or i32 %conv2.i, 96
  %qot793 = ashr exact i32 %conv2.i.3, 1
  %Block2D_AddrPayload_A = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %3, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %Block2D_AddrPayload_B = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %4, i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
  br label %for.body19.i

for.body19.i:
  %5 = shl nuw nsw i32 undef, 1
  %6 = shl nuw nsw i32 undef, 5

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_A, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_A, i32 6, i32 %add58.i, i1 false)
  %Block2D_ReadAddrPayload1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_A, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv.i9.1, i1 false)

  %Block2D_ReadAddrPayload2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload3 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload3, <8 x i32> %Block2D_ReadAddrPayload1, i32 11, i32 11, i32 8, i32 8, i1 false)

  %odd = icmp eq i32 %0, 1
  br i1 %odd, label %dpas_bb, label %backedge

dpas_bb:
  %dpas.1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload2, <8 x i32> %Block2D_ReadAddrPayload1, i32 11, i32 11, i32 8, i32 8, i1 false)
  br label %backedge

backedge:
  br label %for.body19.i
}


declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
