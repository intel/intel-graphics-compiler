;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers -platformpvc \
; RUN:         --regkey LoopSinkEnableVectorShuffle=1,ForceLoopSink=1,LoopSinkForceRollback=1 \
; RUN:         --regkey LoopSinkAvoidSplittingDPAS=0,LoopSinkEnable2dBlockReads=1,LoopSinkEnableLoadsRescheduling=1 \
; RUN:         --regkey CodeSinkingLoadSchedulingInstr=1,LoopSinkCoarserLoadsRescheduling=0,CodeLoopSinkingMinSize=10 \
; RUN:         %enable-basic-aa% --igc-code-loop-sinking --verify -S %s 2>&1 | FileCheck %s

define spir_kernel void @foo(<8 x float> %0, <8 x float> %1, <8 x float> %2, <8 x float> %3, <8 x float> %4, <8 x float> %5) {

; Check nothing is sinked after rollback: first come load and lowered vector shuffle, then DPASes

; CHECK-LABEL: @foo(

; CHECK:         [[BLOCK2D_ADDRPAYLOAD1062:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0)
; CHECK:         br label [[DOT_CRIT_EDGE:%.*]]

; CHECK:       ._crit_edge:
; CHECK:         [[BLOCK2D_READADDRPAYLOAD1065:%.*]] = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(ptr [[BLOCK2D_ADDRPAYLOAD1062]], i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)

; CHECK:         [[TMP12:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 0
; CHECK:         [[TMP13:%.*]] = insertelement <8 x i32> undef, i32 [[TMP12]], i32 0
; CHECK:         [[TMP14:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 2
; CHECK:         [[TMP15:%.*]] = insertelement <8 x i32> [[TMP13]], i32 [[TMP14]], i32 1
; CHECK:         [[TMP16:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 4
; CHECK:         [[TMP17:%.*]] = insertelement <8 x i32> [[TMP15]], i32 [[TMP16]], i32 2
; CHECK:         [[TMP18:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 6
; CHECK:         [[TMP19:%.*]] = insertelement <8 x i32> [[TMP17]], i32 [[TMP18]], i32 3
; CHECK:         [[TMP20:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 8
; CHECK:         [[TMP21:%.*]] = insertelement <8 x i32> [[TMP19]], i32 [[TMP20]], i32 4
; CHECK:         [[TMP22:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 10
; CHECK:         [[TMP23:%.*]] = insertelement <8 x i32> [[TMP21]], i32 [[TMP22]], i32 5
; CHECK:         [[TMP24:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 12
; CHECK:         [[TMP25:%.*]] = insertelement <8 x i32> [[TMP23]], i32 [[TMP24]], i32 6
; CHECK:         [[TMP26:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 14
; CHECK:         [[TMP27:%.*]] = insertelement <8 x i32> [[TMP25]], i32 [[TMP26]], i32 7
; CHECK:         [[TMP28:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 1
; CHECK:         [[TMP29:%.*]] = insertelement <8 x i32> undef, i32 [[TMP28]], i32 0
; CHECK:         [[TMP30:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 3
; CHECK:         [[TMP31:%.*]] = insertelement <8 x i32> [[TMP29]], i32 [[TMP30]], i32 1
; CHECK:         [[TMP32:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 5
; CHECK:         [[TMP33:%.*]] = insertelement <8 x i32> [[TMP31]], i32 [[TMP32]], i32 2
; CHECK:         [[TMP34:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 7
; CHECK:         [[TMP35:%.*]] = insertelement <8 x i32> [[TMP33]], i32 [[TMP34]], i32 3
; CHECK:         [[TMP36:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 9
; CHECK:         [[TMP37:%.*]] = insertelement <8 x i32> [[TMP35]], i32 [[TMP36]], i32 4
; CHECK:         [[TMP38:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 11
; CHECK:         [[TMP39:%.*]] = insertelement <8 x i32> [[TMP37]], i32 [[TMP38]], i32 5
; CHECK:         [[TMP40:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 13
; CHECK:         [[TMP41:%.*]] = insertelement <8 x i32> [[TMP39]], i32 [[TMP40]], i32 6
; CHECK:         [[TMP42:%.*]] = extractelement <16 x i32> [[BLOCK2D_READADDRPAYLOAD1065]], i32 15
; CHECK:         [[TMP43:%.*]] = insertelement <8 x i32> [[TMP41]], i32 [[TMP42]], i32 7

; CHECK:         [[DPAS:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0:%.*]], <8 x i16> [[TMP8:%.*]], <8 x i32> [[TMP10:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS36:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP8]], <8 x i32> [[TMP11:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS37:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP8]], <8 x i32> [[TMP44:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS38:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP8]], <8 x i32> [[TMP45:%.*]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS39:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP9:%.*]], <8 x i32> [[TMP10]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS40:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP9]], <8 x i32> [[TMP11]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS41:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP9]], <8 x i32> [[TMP44]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS42:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]], <8 x i16> [[TMP9]], <8 x i32> [[TMP45]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32
; CHECK:         @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32

; CHECK:         br
;
precompiled_s32divrem.exit1167:
  %Block2D_AddrPayload1062 = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0)
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.._crit_edge_crit_edge, %precompiled_s32divrem.exit1167
  %6 = phi <8 x float> [ zeroinitializer, %precompiled_s32divrem.exit1167 ], [ %dpas52, %._crit_edge.._crit_edge_crit_edge ]
  %7 = phi <8 x float> [ zeroinitializer, %precompiled_s32divrem.exit1167 ], [ %dpas51, %._crit_edge.._crit_edge_crit_edge ]
  %8 = insertelement <8 x i16> zeroinitializer, i16 0, i32 0
  %9 = insertelement <8 x i16> zeroinitializer, i16 0, i32 0
  %10 = insertelement <8 x i32> zeroinitializer, i32 0, i32 0
  %11 = insertelement <8 x i32> zeroinitializer, i32 0, i32 0
  %Block2D_ReadAddrPayload1065 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(ptr %Block2D_AddrPayload1062, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %12 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 0
  %13 = insertelement <8 x i32> undef, i32 %12, i32 0
  %14 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 2
  %15 = insertelement <8 x i32> %13, i32 %14, i32 1
  %16 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 4
  %17 = insertelement <8 x i32> %15, i32 %16, i32 2
  %18 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 6
  %19 = insertelement <8 x i32> %17, i32 %18, i32 3
  %20 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 8
  %21 = insertelement <8 x i32> %19, i32 %20, i32 4
  %22 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 10
  %23 = insertelement <8 x i32> %21, i32 %22, i32 5
  %24 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 12
  %25 = insertelement <8 x i32> %23, i32 %24, i32 6
  %26 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 14
  %27 = insertelement <8 x i32> %25, i32 %26, i32 7
  %28 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 1
  %29 = insertelement <8 x i32> undef, i32 %28, i32 0
  %30 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 3
  %31 = insertelement <8 x i32> %29, i32 %30, i32 1
  %32 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 5
  %33 = insertelement <8 x i32> %31, i32 %32, i32 2
  %34 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 7
  %35 = insertelement <8 x i32> %33, i32 %34, i32 3
  %36 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 9
  %37 = insertelement <8 x i32> %35, i32 %36, i32 4
  %38 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 11
  %39 = insertelement <8 x i32> %37, i32 %38, i32 5
  %40 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 13
  %41 = insertelement <8 x i32> %39, i32 %40, i32 6
  %42 = extractelement <16 x i32> %Block2D_ReadAddrPayload1065, i32 15
  %43 = insertelement <8 x i32> %41, i32 %42, i32 7
  %44 = insertelement <8 x i32> zeroinitializer, i32 0, i32 0
  %45 = insertelement <8 x i32> zeroinitializer, i32 0, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %8, <8 x i32> %10, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas36 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %8, <8 x i32> %11, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas37 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %8, <8 x i32> %44, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas38 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %8, <8 x i32> %45, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas39 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %9, <8 x i32> %10, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas40 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %9, <8 x i32> %11, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas41 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %9, <8 x i32> %44, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas42 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %0, <8 x i16> %9, <8 x i32> %45, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas51 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %7, <8 x i16> zeroinitializer, <8 x i32> %27, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas52 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %6, <8 x i16> zeroinitializer, <8 x i32> %43, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge.._crit_edge_crit_edge

._crit_edge.._crit_edge_crit_edge:                ; preds = %._crit_edge
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(ptr, i32, i32, i1)

declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{}
