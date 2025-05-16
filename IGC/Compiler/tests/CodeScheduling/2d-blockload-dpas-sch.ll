;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, system-linux

; Larger test case that checks some basic scheduling behavior for 2d block loads and dpas instructions
; And also checks
;   - that the debug info doesn't affect the scheduling
;   - the determinism of the pass
;   - the produced IR is correct (--verify)
;   - the debug info is preserved


; no-debug run
; RUN: igc_opt --opaque-pointers -platformpvc --regkey DisableLoopSink=1 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --igc-code-scheduling --verify \
; RUN:         -S %s &> %t.no_debug.ll

; no-debug run duplicate to check determinism
; RUN: igc_opt --opaque-pointers -platformpvc --regkey DisableLoopSink=1 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --igc-code-scheduling --verify \
; RUN:         -S %s &> %t.no_debug.2.ll

; Check that the two runs produce the same output
; RUN: diff %t.no_debug.ll %t.no_debug.2.ll

; debug run + check the debugify check pass
; RUN: igc_opt --opaque-pointers -platformpvc --regkey DisableLoopSink=1 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --debugify --igc-code-scheduling --verify --check-debugify \
; RUN:         -S %s > %t.with_debug.ll 2> %t.with_debug.debugify.log

; RUN: FileCheck %s --check-prefix=CHECK-DEBUGIFY --input-file=%t.with_debug.debugify.log
; CHECK-DEBUGIFY: CheckModuleDebugify: PASS

; RUN: igc_opt --opaque-pointers --strip-debug -S %t.with_debug.ll &> %t.with_debug.stripped.ll

; Check basic scheduling behavior for 2d block loads and dpas instructions.
; Also check that the debug info doesn't affect the scheduling

; RUN: FileCheck %s --check-prefix=CHECK --input-file=%t.no_debug.ll
; RUN: FileCheck %s --check-prefix=CHECK --input-file=%t.with_debug.stripped.ll


define spir_kernel void @no_barrier(ptr addrspace(1) %_arg_A, ptr addrspace(1) %_arg_B, i16 %localIdY) {
; scheduling with no barrier:
; BLOCK2D_READADDRPAYLOAD1, BLOCK2D_READADDRPAYLOAD2, BLOCK2D_READADDRPAYLOAD3,
;     dpas.1.1, dpas.2.1 (2 DPAS that increase the regpressure but are ready earlier),
;     dpas.1.2, dpas.2.2, BLOCK2D_READADDRPAYLOAD4 (no deps)


; CHECK-LABEL: @no_barrier(
; CHECK:       __igcbuiltin_u64_udiv_dp.exit:
; CHECK:         [[BLOCK2D_ADDRPAYLOAD_A:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 [[TMP3:%.*]], i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
; CHECK:         [[BLOCK2D_ADDRPAYLOAD_B:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 [[TMP4:%.*]], i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
; CHECK:         br label [[FOR_BODY19_I:%.*]]
; CHECK:       for.body19.i:
; CHECK:         [[TMP5:%.*]] = shl nuw nsw i32 undef, 5
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_A]], i32 5, i32 [[TMP5]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_A]], i32 6, i32 [[ADD58_I:%.*]], i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[BLOCK2D_ADDRPAYLOAD_A]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 5, i32 [[TMP5]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 6, i32 [[CONV_I9_1:%.*]], i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD2:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         [[EE1:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 0
; CHECK:         [[NEWVEC:%.*]] = insertelement <8 x i16> undef, i16 [[EE1]], i32 7
; CHECK:         [[EE2:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 1
; CHECK:         [[NEWVEC1:%.*]] = insertelement <8 x i16> [[NEWVEC]], i16 [[EE2]], i32 6
; CHECK:         [[EE3:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 2
; CHECK:         [[NEWVEC2:%.*]] = insertelement <8 x i16> [[NEWVEC1]], i16 [[EE3]], i32 5
; CHECK:         [[EE4:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 3
; CHECK:         [[NEWVEC3:%.*]] = insertelement <8 x i16> [[NEWVEC2]], i16 [[EE4]], i32 4
; CHECK:         [[EE5:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 4
; CHECK:         [[NEWVEC4:%.*]] = insertelement <8 x i16> [[NEWVEC3]], i16 [[EE5]], i32 3
; CHECK:         [[EE6:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 5
; CHECK:         [[NEWVEC5:%.*]] = insertelement <8 x i16> [[NEWVEC4]], i16 [[EE6]], i32 2
; CHECK:         [[EE7:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 6
; CHECK:         [[NEWVEC6:%.*]] = insertelement <8 x i16> [[NEWVEC5]], i16 [[EE7]], i32 1
; CHECK:         [[EE8:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 7
; CHECK:         [[NEWVEC7:%.*]] = insertelement <8 x i16> [[NEWVEC6]], i16 [[EE8]], i32 0
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 5, i32 [[QOT781:%.*]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 6, i32 [[CONV2_I_3:%.*]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 2, i32 243, i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD3:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         [[DPAS_1_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD1]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS_2_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD1]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 5, i32 [[QOT793:%.*]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 6, i32 [[CONV2_I_3]], i1 false)
; CHECK:         [[DPAS_1_2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[DPAS_1_1]], <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD3]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS_2_2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[DPAS_2_1]], <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD3]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[TMP6:%.*]] = shl nuw nsw i32 undef, 1
; CHECK:         [[BLOCK2D_READADDRPAYLOAD4:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         br label [[FOR_BODY19_I]]
;

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

for.body19.i:                                     ; preds = %for.body19.i.for.body19.i_crit_edge, %__igcbuiltin_u64_udiv_dp.exit
  %5 = shl nuw nsw i32 undef, 1
  %6 = shl nuw nsw i32 undef, 5

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_A, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_A, i32 6, i32 %add58.i, i1 false)
  %Block2D_ReadAddrPayload1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_A, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv.i9.1, i1 false)
  %Block2D_ReadAddrPayload2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %EE1 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 0
  %newvec = insertelement <8 x i16> undef, i16 %EE1, i32 7
  %EE2 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 1
  %newvec1 = insertelement <8 x i16> %newvec, i16 %EE2, i32 6
  %EE3 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 2
  %newvec2 = insertelement <8 x i16> %newvec1, i16 %EE3, i32 5
  %EE4 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 3
  %newvec3 = insertelement <8 x i16> %newvec2, i16 %EE4, i32 4
  %EE5 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 4
  %newvec4 = insertelement <8 x i16> %newvec3, i16 %EE5, i32 3
  %EE6 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 5
  %newvec5 = insertelement <8 x i16> %newvec4, i16 %EE6, i32 2
  %EE7 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 6
  %newvec6 = insertelement <8 x i16> %newvec5, i16 %EE7, i32 1
  %EE8 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 7
  %newvec7 = insertelement <8 x i16> %newvec6, i16 %EE8, i32 0


  %dpas.1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload1, i32 11, i32 11, i32 8, i32 8, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %qot781, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv2.i.3, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 2, i32 243, i1 false)
  %Block2D_ReadAddrPayload3 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %dpas.1.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas.1.1, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload3, i32 11, i32 11, i32 8, i32 8, i1 false)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %qot793, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv2.i.3, i1 false)
  %Block2D_ReadAddrPayload4 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %dpas.2.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas.2.1, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload3, i32 11, i32 11, i32 8, i32 8, i1 false)

  br label %for.body19.i
}


; Check the barrier is being respected

define spir_kernel void @with_barrier(ptr addrspace(1) %_arg_A, ptr addrspace(1) %_arg_B, i16 %localIdY) {

; scheduling with barrier:
; BLOCK2D_READADDRPAYLOAD1, BLOCK2D_READADDRPAYLOAD2, BLOCK2D_READADDRPAYLOAD3,
;     dpas.1.1, dpas.1.2 (can's move DPAS 2.1 earlier because of the barrier), barrier
;     dpas.2.1, dpas.2.2, BLOCK2D_READADDRPAYLOAD4 (no deps)

; CHECK-LABEL: @with_barrier(
; CHECK:       __igcbuiltin_u64_udiv_dp.exit:
; CHECK:         [[BLOCK2D_ADDRPAYLOAD_A:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 [[TMP3:%.*]], i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
; CHECK:         [[BLOCK2D_ADDRPAYLOAD_B:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 [[TMP4:%.*]], i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
; CHECK:         br label [[FOR_BODY19_I:%.*]]
; CHECK:       for.body19.i:
; CHECK:         [[TMP5:%.*]] = shl nuw nsw i32 undef, 5
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_A]], i32 5, i32 [[TMP5]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_A]], i32 6, i32 [[ADD58_I]], i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD1:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[BLOCK2D_ADDRPAYLOAD_A]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 5, i32 [[TMP5]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 6, i32 [[CONV_I9_1]], i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD2:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         [[EE1:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 0
; CHECK:         [[NEWVEC:%.*]] = insertelement <8 x i16> undef, i16 [[EE1]], i32 7
; CHECK:         [[EE2:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 1
; CHECK:         [[NEWVEC1:%.*]] = insertelement <8 x i16> [[NEWVEC]], i16 [[EE2]], i32 6
; CHECK:         [[EE3:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 2
; CHECK:         [[NEWVEC2:%.*]] = insertelement <8 x i16> [[NEWVEC1]], i16 [[EE3]], i32 5
; CHECK:         [[EE4:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 3
; CHECK:         [[NEWVEC3:%.*]] = insertelement <8 x i16> [[NEWVEC2]], i16 [[EE4]], i32 4
; CHECK:         [[EE5:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 4
; CHECK:         [[NEWVEC4:%.*]] = insertelement <8 x i16> [[NEWVEC3]], i16 [[EE5]], i32 3
; CHECK:         [[EE6:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 5
; CHECK:         [[NEWVEC5:%.*]] = insertelement <8 x i16> [[NEWVEC4]], i16 [[EE6]], i32 2
; CHECK:         [[EE7:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 6
; CHECK:         [[NEWVEC6:%.*]] = insertelement <8 x i16> [[NEWVEC5]], i16 [[EE7]], i32 1
; CHECK:         [[EE8:%.*]] = extractelement <8 x i16> [[BLOCK2D_READADDRPAYLOAD2]], i32 7
; CHECK:         [[NEWVEC7:%.*]] = insertelement <8 x i16> [[NEWVEC6]], i16 [[EE8]], i32 0
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 5, i32 [[QOT781]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 6, i32 [[CONV2_I_3]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 2, i32 243, i1 false)
; CHECK:         [[BLOCK2D_READADDRPAYLOAD3:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         [[DPAS_1_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD1]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[DPAS_1_2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[DPAS_1_1]], <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD3]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         call void @foo()
; CHECK:         call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
; CHECK:         call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK:         [[DPAS_2_1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD1]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 5, i32 [[QOT793]], i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 6, i32 [[CONV2_I_3]], i1 false)
; CHECK:         [[DPAS_2_2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[DPAS_2_1]], <8 x i16> [[NEWVEC7]], <8 x i32> [[BLOCK2D_READADDRPAYLOAD3]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:         [[TMP6:%.*]] = shl nuw nsw i32 undef, 1
; CHECK:         [[X7:%.*]] = shl nuw nsw i32 undef, 1
; CHECK:         [[BLOCK2D_READADDRPAYLOAD4:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[BLOCK2D_ADDRPAYLOAD_B]], i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:         br label [[FOR_BODY19_I]]
;

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

for.body19.i:                                     ; preds = %for.body19.i.for.body19.i_crit_edge, %__igcbuiltin_u64_udiv_dp.exit
  %5 = shl nuw nsw i32 undef, 1
  %6 = shl nuw nsw i32 undef, 5

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_A, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_A, i32 6, i32 %add58.i, i1 false)
  %Block2D_ReadAddrPayload1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_A, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %6, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv.i9.1, i1 false)
  %Block2D_ReadAddrPayload2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %EE1 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 0
  %newvec = insertelement <8 x i16> undef, i16 %EE1, i32 7
  %EE2 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 1
  %newvec1 = insertelement <8 x i16> %newvec, i16 %EE2, i32 6
  %EE3 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 2
  %newvec2 = insertelement <8 x i16> %newvec1, i16 %EE3, i32 5
  %EE4 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 3
  %newvec3 = insertelement <8 x i16> %newvec2, i16 %EE4, i32 4
  %EE5 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 4
  %newvec4 = insertelement <8 x i16> %newvec3, i16 %EE5, i32 3
  %EE6 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 5
  %newvec5 = insertelement <8 x i16> %newvec4, i16 %EE6, i32 2
  %EE7 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 6
  %newvec6 = insertelement <8 x i16> %newvec5, i16 %EE7, i32 1
  %EE8 = extractelement <8 x i16> %Block2D_ReadAddrPayload2, i32 7
  %newvec7 = insertelement <8 x i16> %newvec6, i16 %EE8, i32 0


  %dpas.1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload1, i32 11, i32 11, i32 8, i32 8, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %qot781, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv2.i.3, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 2, i32 243, i1 false)
  %Block2D_ReadAddrPayload3 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %dpas.1.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas.1.1, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload3, i32 11, i32 11, i32 8, i32 8, i1 false)

  call void @foo()
  %x7 = shl nuw nsw i32 undef, 1
  call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 5, i32 %qot793, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %Block2D_AddrPayload_B, i32 6, i32 %conv2.i.3, i1 false)
  %Block2D_ReadAddrPayload4 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %Block2D_AddrPayload_B, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %dpas.2.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas.2.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas.2.1, <8 x i16> %newvec7, <8 x i32> %Block2D_ReadAddrPayload3, i32 11, i32 11, i32 8, i32 8, i1 false)

  br label %for.body19.i
}


declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()
declare void @foo()

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
