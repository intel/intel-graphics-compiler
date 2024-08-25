;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt -platformpvc --regkey LoopSinkForceRollback=1 --regkey LoopSinkMinSave=1 --regkey LoopSinkEnable2dBlockReads=1 --regkey LoopSinkEnableLoadsRescheduling=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey CodeSinkingLoadSchedulingInstr=10 %enable-basic-aa% --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s


; Check that the order of the first loads and SetField calls is not changed after rollback

; CHECK-LABEL: @foo(
; CHECK:       __igcbuiltin_u64_udiv_dp.exit:
; CHECK:         [[MUL56_I:%.*]] = shl i32 0, 8
; CHECK:         [[TMP0:%.*]] = zext i16 [[LOCALIDY:%.*]] to i32
; CHECK:         [[MUL57_I:%.*]] = shl nuw nsw i32 [[TMP0]], 5
; CHECK:         [[ADD58_I:%.*]] = add i32 [[MUL56_I]], [[MUL57_I]]
; CHECK:         [[CONV_I9_1:%.*]] = or i32 [[ADD58_I]], 8
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 [[TMP3:.*]], i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
; CHECK:         [[BLOCK2D_ADDRPAYLOAD110:%.*]] = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 [[TMP4:.*]], i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
; CHECK:         br label [[FOR_BODY19_I:%.*]]
; CHECK:       for.body19.i:
; CHECK:         [[TMP6:%.*]] = shl nuw nsw i32 undef, 5
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 [[TMP6]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 [[ADD58_I]], i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 [[TMP6]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 [[CONV_I9_1]], i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD105:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)


define spir_kernel void @foo(i8 addrspace(1)* %_arg_A, i8 addrspace(1)* %_arg_B, i16 %localIdY) {
__igcbuiltin_u64_udiv_dp.exit:
  %mul56.i = shl i32 0, 8
  %0 = zext i16 %localIdY to i32
  %mul57.i = shl nuw nsw i32 %0, 5
  %add58.i = add i32 %mul56.i, %mul57.i
  %mul83.i = shl nuw nsw i64 0, 7
  %1 = shl nuw nsw i64 0, 9
  %2 = add nuw nsw i64 %1, %mul83.i
  %.ascast.i67 = addrspacecast i8 addrspace(1)* %_arg_A to i8 addrspace(4)*
  %3 = ptrtoint i8 addrspace(4)* %.ascast.i67 to i64
  %.ascast.i68 = addrspacecast i8 addrspace(1)* %_arg_A to i8 addrspace(4)*
  %4 = ptrtoint i8 addrspace(4)* %.ascast.i68 to i64
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
  %Block2D_AddrPayload = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 %3, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %Block2D_AddrPayload110 = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 %4, i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
  br label %for.body19.i

for.body19.i:                                     ; preds = %for.body19.i.for.body19.i_crit_edge, %__igcbuiltin_u64_udiv_dp.exit
  %5 = shl nuw nsw i32 undef, 1
  %6 = shl nuw nsw i32 undef, 5

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 6, i32 %add58.i, i1 false)
  %Block2D_ReadAddrPayload = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 6, i32 %conv.i9.1, i1 false)
  %Block2D_ReadAddrPayload105 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 6, i32 %conv.i9.2, i1 false)
  %Block2D_ReadAddrPayload107 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 6, i32 %conv.i9.3, i1 false)
  %Block2D_ReadAddrPayload109 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %7 = shl nuw nsw i32 undef, 4
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 5, i32 %qot, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload111 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 5, i32 %qot781, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload113 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 5, i32 %qot787, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload115 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 5, i32 %qot793, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload117 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload111, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload113, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload115, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload117, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.143 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload105, <8 x i32> %Block2D_ReadAddrPayload111, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload105, <8 x i32> %Block2D_ReadAddrPayload113, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload105, <8 x i32> %Block2D_ReadAddrPayload115, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.3.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload105, <8 x i32> %Block2D_ReadAddrPayload117, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.244 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload107, <8 x i32> %Block2D_ReadAddrPayload111, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.1.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload107, <8 x i32> %Block2D_ReadAddrPayload113, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload107, <8 x i32> %Block2D_ReadAddrPayload115, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.3.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload107, <8 x i32> %Block2D_ReadAddrPayload117, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.345 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload109, <8 x i32> %Block2D_ReadAddrPayload111, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.1.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload109, <8 x i32> %Block2D_ReadAddrPayload113, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload109, <8 x i32> %Block2D_ReadAddrPayload115, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.3.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload109, <8 x i32> %Block2D_ReadAddrPayload117, i32 11, i32 11, i32 8, i32 8, i1 false)
  br label %for.body19.i
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32*, i32, i32, i1)

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
