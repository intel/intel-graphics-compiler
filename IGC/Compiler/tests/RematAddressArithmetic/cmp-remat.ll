;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers %s -S -o - -igc-clone-address-arithmetic  --regkey=RematFlowThreshold=20 --regkey=RematRPELimit=0 --dce | FileCheck %s

; Function Attrs: convergent nounwind null_pointer_is_valid
define spir_kernel void @widget(half addrspace(1)* align 2 %arg, half addrspace(1)* align 2 %arg1, half addrspace(1)* align 2 %arg2, float addrspace(1)* nocapture align 4 %arg3, i8 addrspace(1)* nocapture readonly align 1 %arg4, i32 addrspace(1)* nocapture readonly align 4 %arg5, i8 addrspace(1)* nocapture readonly align 1 %arg6, i32 addrspace(1)* nocapture readonly align 4 %arg7, half addrspace(1)* align 2 %arg8, <8 x i32> %arg9, i16 %arg10) #0 {
bb:
  %tmp = extractelement <8 x i32> %arg9, i64 1
  %tmp11 = extractelement <8 x i32> %arg9, i64 6
  %tmp12 = icmp slt i32 %tmp11, 0
  %tmp14 = add i32 %tmp11, 127
  %spec.select = select i1 %tmp12, i32 %tmp14, i32 %tmp11
  %tmp17 = ashr i32 %spec.select, 7
  %tmp18 = shl i32 %tmp17, 7
  %tmp19 = sub i32 %tmp11, %tmp18
  %tmp20 = mul i32 %tmp17, 25165824
  %tmp21 = mul nsw i32 %tmp19, 196608
  %tmp22 = add i32 %tmp20, %tmp21
  %tmp23 = shl nsw i32 %tmp19, 17
  %tmp24 = sext i32 %tmp22 to i64
  %tmp25 = ptrtoint half addrspace(1)* %arg to i64
  %tmp26 = shl nsw i64 %tmp24, 1
  %tmp27 = add i64 %tmp26, %tmp25
  %tmp28 = sext i32 %tmp21 to i64
  %tmp29 = ptrtoint half addrspace(1)* %arg1 to i64
  %tmp30 = shl nsw i64 %tmp28, 1
  %tmp31 = add i64 %tmp30, %tmp29
  %tmp32 = sext i32 %tmp23 to i64
  %tmp33 = ptrtoint half addrspace(1)* %arg2 to i64
  %tmp34 = shl nsw i64 %tmp32, 1
  %tmp35 = add i64 %tmp34, %tmp33
  %tmp36 = shl i32 %tmp, 7
  %tmp37 = zext i16 %arg10 to i32
  %tmp38 = and i32 %tmp37, 112
  %tmp39 = or i32 %tmp38, %tmp36
  %tmp40 = or i32 %tmp39, 1
  %tmp41 = or i32 %tmp39, 2
  %tmp42 = or i32 %tmp39, 4
  %tmp43 = or i32 %tmp39, 5
  %tmp44 = or i32 %tmp39, 6
  %tmp45 = or i32 %tmp39, 7
  %tmp48 = and i64 %tmp27, -64
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %tmp48, i32 0, i32 1023, i32 383, i32 0, i32 %tmp36, i32 16, i32 32, i32 32, i32 1, i1 false, i1 false, i32 4)
  %tmp51 = and i64 %tmp31, -64
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %tmp51, i32 0, i32 1023, i32 383, i32 0, i32 0, i32 16, i32 32, i32 32, i32 1, i1 false, i1 false, i32 4)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %tmp51, i32 0, i32 1023, i32 383, i32 0, i32 32, i32 16, i32 32, i32 32, i32 1, i1 false, i1 false, i32 4)
  %tmp54 = and i64 %tmp35, -64
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %tmp54, i32 255, i32 1023, i32 255, i32 0, i32 0, i32 16, i32 32, i32 32, i32 1, i1 false, i1 false, i32 4)
  br label %bb57

bb57:                                             ; preds = %bb57, %bb
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* null, i32 6, i32 0, i1 false)
  %tmp58 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* null, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %tmp59 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* null, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* null, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* null, i32 6, i32 0, i1 false)
  %tmp60 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* null, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %tmp61 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* null, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* null, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* null, i32 6, i32 0, i1 false)
  %tmp62 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* null, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %tmp63 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32* null, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* null, i32 5, i32 0, i1 false)
  %tmp84 = icmp slt i32 %tmp40, 0
  %tmp85 = icmp slt i32 %tmp41, 0
  %tmp87 = icmp slt i32 %tmp42, 0
  %tmp88 = icmp slt i32 %tmp43, 0
  %tmp89 = icmp slt i32 %tmp44, 0
  %tmp90 = icmp slt i32 %tmp45, 0

; CHECK:  [[REMAT_1:%remat.*]] = or i32 %tmp39, 1
; CHECK:  [[CLONED_1:%cloned_.*]] = icmp slt i32 [[REMAT_1]]
; CHECK:  = select i1 [[CLONED_1]]
; CHECK:  [[REMAT_2:%remat.*]] = or i32 %tmp39, 2
; CHECK:  [[CLONED_2:%cloned_.*]] = icmp slt i32 [[REMAT_2]]
; CHECK:  = select i1 [[CLONED_2]]

  %tmp95 = select i1 %tmp84, float 0xFFF0000000000000, float 0.000000e+00
  %tmp96 = select i1 %tmp85, float 0xFFF0000000000000, float 0.000000e+00
  %tmp98 = select i1 %tmp87, float 0xFFF0000000000000, float 0.000000e+00
  %tmp99 = select i1 %tmp88, float 0xFFF0000000000000, float 0.000000e+00
  %tmp100 = select i1 %tmp89, float 0xFFF0000000000000, float 0.000000e+00
  %tmp101 = select i1 %tmp90, float 0xFFF0000000000000, float 0.000000e+00
  %tmp103 = insertelement <8 x float> <float 0.000000e+00, float undef, float undef, float undef, float undef, float undef, float undef, float undef>, float %tmp95, i64 1
  %tmp104 = insertelement <8 x float> %tmp103, float %tmp96, i64 2
  %tmp105 = insertelement <8 x float> %tmp104, float 0.000000e+00, i64 3
  %tmp106 = insertelement <8 x float> %tmp105, float %tmp98, i64 4
  %tmp107 = insertelement <8 x float> %tmp106, float %tmp99, i64 5
  %tmp108 = insertelement <8 x float> %tmp107, float %tmp100, i64 6
  %tmp109 = insertelement <8 x float> %tmp108, float %tmp101, i64 7
  %tmp110 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> zeroinitializer, <8 x float> %tmp109)
  %tmp111 = extractelement <8 x float> %tmp110, i64 0
  %tmp118 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp111, i8 12, i1 true, i32 0)
  br label %bb57
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #1

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #2

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #3

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.umin.i32(i32, i32) #4

; Function Attrs: nounwind readnone speculatable willreturn
declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32) #5

; Function Attrs: argmemonly nounwind speculatable willreturn writeonly
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32*, i32, i32, i1) #6

; Function Attrs: nounwind willreturn
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind willreturn
declare <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind willreturn
declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #4

attributes #0 = { convergent nounwind null_pointer_is_valid }
attributes #1 = { convergent inaccessiblememonly nounwind }
attributes #2 = { convergent nounwind readnone willreturn }
attributes #3 = { nounwind }
attributes #4 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #5 = { nounwind readnone speculatable willreturn }
attributes #6 = { argmemonly nounwind speculatable willreturn writeonly }
attributes #7 = { nounwind willreturn }

!igc.functions = !{}
