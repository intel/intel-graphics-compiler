;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers -platformpvc --regkey LoopSinkMinSave=1 --regkey LoopSinkAvoidSplittingDPAS=0 --regkey LoopSinkEnable2dBlockReads=1 --regkey LoopSinkEnableLoadsRescheduling=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey CodeSinkingLoadSchedulingInstr=1 --regkey LoopSinkCoarserLoadsRescheduling=0 --regkey LoopSinkEnableVectorShuffle=1 %enable-basic-aa% --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s

define spir_kernel void @foo(ptr addrspace(1) %_arg_A, ptr addrspace(1) %_arg_B, i16 %localIdY) {
; Check that the order of the first loads and SetField calls is not changed after rollback
; CHECK-LABEL: @foo(
; CHECK:       for.body19.i:

; First we have some DPASes

; CHECK:              [[DPAS:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD:%.*]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD111:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:              [[DPAS_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD113:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:              [[DPAS_2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD115:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:              [[DPAS_3:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD117:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)

; Ensure that the load + vector shuffle pattern is scheduled exactly before the DPAS that uses its result

; CHECK:              [[SCHED_BLOCK2D_READADDRPAYLOAD105:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[BLOCK2D_ADDRPAYLOAD:%.*]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK-NEXT:         [[SCHED_EE1:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 0
; CHECK-NEXT:         [[SCHED_NEWVEC:%.*]] = insertelement <8 x i16> undef, i16 [[SCHED_EE1]], i32 7
; CHECK-NEXT:         [[SCHED_EE2:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 1
; CHECK-NEXT:         [[SCHED_NEWVEC1:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC]], i16 [[SCHED_EE2]], i32 6
; CHECK-NEXT:         [[SCHED_EE3:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 2
; CHECK-NEXT:         [[SCHED_NEWVEC2:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC1]], i16 [[SCHED_EE3]], i32 5
; CHECK-NEXT:         [[SCHED_EE4:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 3
; CHECK-NEXT:         [[SCHED_NEWVEC3:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC2]], i16 [[SCHED_EE4]], i32 4
; CHECK-NEXT:         [[SCHED_EE5:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 4
; CHECK-NEXT:         [[SCHED_NEWVEC4:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC3]], i16 [[SCHED_EE5]], i32 3
; CHECK-NEXT:         [[SCHED_EE6:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 5
; CHECK-NEXT:         [[SCHED_NEWVEC5:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC4]], i16 [[SCHED_EE6]], i32 2
; CHECK-NEXT:         [[SCHED_EE7:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 6
; CHECK-NEXT:         [[SCHED_NEWVEC6:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC5]], i16 [[SCHED_EE7]], i32 1
; CHECK-NEXT:         [[SCHED_EE8:%.*]] = extractelement <8 x i16> [[SCHED_BLOCK2D_READADDRPAYLOAD105]], i32 7
; CHECK-NEXT:         [[SCHED_NEWVEC7:%.*]] = insertelement <8 x i16> [[SCHED_NEWVEC6]], i16 [[SCHED_EE8]], i32 0
; CHECK-NEXT:         [[DPAS_143:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_NEWVEC7]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD111]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK-NEXT:         [[DPAS_1_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_NEWVEC7]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD113]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK-NEXT:         [[DPAS_2_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_NEWVEC7]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD115]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK-NEXT:         [[DPAS_3_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[SCHED_NEWVEC7]], <8 x i32> [[SCHED_BLOCK2D_READADDRPAYLOAD117]], i32 11, i32 11, i32 8, i32 8, i1 false)

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
  %Block2D_AddrPayload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %3, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %Block2D_AddrPayload110 = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %4, i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
  br label %for.body19.i

for.body19.i:                                     ; preds = %for.body19.i.for.body19.i_crit_edge, %__igcbuiltin_u64_udiv_dp.exit
  %5 = shl nuw nsw i32 undef, 1
  %6 = shl nuw nsw i32 undef, 5

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 6, i32 %add58.i, i1 false)
  %Block2D_ReadAddrPayload = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 6, i32 %conv.i9.1, i1 false)
  %Block2D_ReadAddrPayload105 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %EE1 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 0
  %newvec = insertelement <8 x i16> undef, i16 %EE1, i32 7
  %EE2 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 1
  %newvec1 = insertelement <8 x i16> %newvec, i16 %EE2, i32 6
  %EE3 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 2
  %newvec2 = insertelement <8 x i16> %newvec1, i16 %EE3, i32 5
  %EE4 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 3
  %newvec3 = insertelement <8 x i16> %newvec2, i16 %EE4, i32 4
  %EE5 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 4
  %newvec4 = insertelement <8 x i16> %newvec3, i16 %EE5, i32 3
  %EE6 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 5
  %newvec5 = insertelement <8 x i16> %newvec4, i16 %EE6, i32 2
  %EE7 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 6
  %newvec6 = insertelement <8 x i16> %newvec5, i16 %EE7, i32 1
  %EE8 = extractelement <8 x i16> %Block2D_ReadAddrPayload105, i32 7
  %newvec7 = insertelement <8 x i16> %newvec6, i16 %EE8, i32 0

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 6, i32 %conv.i9.2, i1 false)
  %Block2D_ReadAddrPayload107 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload, i32 6, i32 %conv.i9.3, i1 false)
  %Block2D_ReadAddrPayload109 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %7 = shl nuw nsw i32 undef, 4
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 5, i32 %qot, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload111 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 5, i32 %qot781, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload113 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 5, i32 %qot787, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload115 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 5, i32 %qot793, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload110, i32 6, i32 %7, i1 false)
  %Block2D_ReadAddrPayload117 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload110, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload111, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload113, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload115, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %Block2D_ReadAddrPayload, <8 x i32> %Block2D_ReadAddrPayload117, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.143 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload111, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload113, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload115, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.3.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload117, i32 11, i32 11, i32 8, i32 8, i1 false)
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

declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
