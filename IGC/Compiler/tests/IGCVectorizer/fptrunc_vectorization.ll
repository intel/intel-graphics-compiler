; RUN: igc_opt --igc-vectorizer -S -dce < %s 2>&1 | FileCheck %s

; CHECK: [[VECPHI:%vectorized_phi]] = phi <8 x float> [ zeroinitializer, [[B1:%.*]] ], [ [[INDVAR:%.*]], [[B2:%.*]] ]
; CHECK: [[DPAS0:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[VECPHI]]
; CHECK: [[INDVAR]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[DPAS0]]
; CHECK: [[A:%vectorized_cast.*]] = fptrunc <8 x float> [[INDVAR]] to <8 x half>
; CHECK: [[B:%.*]] = bitcast <8 x half> [[A]] to <8 x i16>
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16({{.*}}, <8 x i16> [[B]])


; ModuleID = 'final_test_ftrunc.ll'
source_filename = "final_test_ftrunc.ll"

define spir_kernel void @pluto(i64 %arg, i64 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7, i32 %arg8, i32 %arg9, i32 %arg10, i32 %arg11) {
bb:
  br label %bb12

bb12:                                             ; preds = %bb12, %bb
  %tmp = phi float [ 0.000000e+00, %bb ], [ %tmp40, %bb12 ]
  %tmp13 = phi float [ 0.000000e+00, %bb ], [ %tmp41, %bb12 ]
  %tmp14 = phi float [ 0.000000e+00, %bb ], [ %tmp42, %bb12 ]
  %tmp15 = phi float [ 0.000000e+00, %bb ], [ %tmp43, %bb12 ]
  %tmp16 = phi float [ 0.000000e+00, %bb ], [ %tmp44, %bb12 ]
  %tmp17 = phi float [ 0.000000e+00, %bb ], [ %tmp45, %bb12 ]
  %tmp18 = phi float [ 0.000000e+00, %bb ], [ %tmp46, %bb12 ]
  %tmp19 = phi float [ 0.000000e+00, %bb ], [ %tmp47, %bb12 ]
  %tmp20 = phi i32 [ 0, %bb ], [ %tmp48, %bb12 ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %arg, i32 8191, i32 4095, i32 %arg3, i32 %arg2, i32 %arg5, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  %tmp21 = or i32 %arg6, %arg7
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %arg1, i32 8191, i32 4095, i32 %arg4, i32 %arg8, i32 %tmp21, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  %tmp22 = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 %arg, i32 8191, i32 4095, i32 %arg3, i32 0, i32 0, i32 16, i32 32, i32 2)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %tmp22, i32 5, i32 %tmp20, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %tmp22, i32 6, i32 %arg9, i1 false)
  %tmp23 = call <64 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v64i16.p0i32(i32* %tmp22, i32 0, i32 0, i32 16, i32 16, i32 32, i32 2, i1 false, i1 false, i32 0)
  %tmp24 = shufflevector <64 x i16> %tmp23, <64 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %tmp25 = shufflevector <64 x i16> %tmp23, <64 x i16> undef, <8 x i32> <i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39>
  %tmp26 = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 %arg1, i32 8191, i32 4095, i32 %arg4, i32 0, i32 0, i32 16, i32 32, i32 2)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %tmp26, i32 5, i32 %arg10, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %tmp26, i32 6, i32 %arg11, i1 false)
  %tmp27 = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0i32(i32* %tmp26, i32 0, i32 0, i32 16, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
  %tmp28 = shufflevector <32 x i32> %tmp27, <32 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %tmp29 = shufflevector <32 x i32> %tmp27, <32 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %tmp30 = insertelement <8 x float> zeroinitializer, float %tmp, i64 0
  %tmp31 = insertelement <8 x float> %tmp30, float %tmp13, i64 1
  %tmp32 = insertelement <8 x float> %tmp31, float %tmp14, i64 2
  %tmp33 = insertelement <8 x float> %tmp32, float %tmp15, i64 3
  %tmp34 = insertelement <8 x float> %tmp33, float %tmp16, i64 4
  %tmp35 = insertelement <8 x float> %tmp34, float %tmp17, i64 5
  %tmp36 = insertelement <8 x float> %tmp35, float %tmp18, i64 6
  %tmp37 = insertelement <8 x float> %tmp36, float %tmp19, i64 7
  %tmp38 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp37, <8 x i16> %tmp24, <8 x i32> %tmp28, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp39 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp38, <8 x i16> %tmp25, <8 x i32> %tmp29, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp40 = extractelement <8 x float> %tmp39, i64 0
  %tmp41 = extractelement <8 x float> %tmp39, i64 1
  %tmp42 = extractelement <8 x float> %tmp39, i64 2
  %tmp43 = extractelement <8 x float> %tmp39, i64 3
  %tmp44 = extractelement <8 x float> %tmp39, i64 4
  %tmp45 = extractelement <8 x float> %tmp39, i64 5
  %tmp46 = extractelement <8 x float> %tmp39, i64 6
  %tmp47 = extractelement <8 x float> %tmp39, i64 7
  %tmp48 = add nuw nsw i32 %tmp20, 32
  %tmp49 = icmp ult i32 %tmp20, 4064
  br i1 %tmp49, label %bb12, label %bb50

bb50:                                             ; preds = %bb12
  %tmp51 = fptrunc float %tmp40 to half
  %tmp52 = fptrunc float %tmp41 to half
  %tmp53 = fptrunc float %tmp42 to half
  %tmp54 = fptrunc float %tmp43 to half
  %tmp55 = fptrunc float %tmp44 to half
  %tmp56 = fptrunc float %tmp45 to half
  %tmp57 = fptrunc float %tmp46 to half
  %tmp58 = fptrunc float %tmp47 to half
  %tmp59 = insertelement <8 x half> zeroinitializer, half %tmp51, i64 0
  %tmp60 = insertelement <8 x half> %tmp59, half %tmp52, i64 1
  %tmp61 = insertelement <8 x half> %tmp60, half %tmp53, i64 2
  %tmp62 = insertelement <8 x half> %tmp61, half %tmp54, i64 3
  %tmp63 = insertelement <8 x half> %tmp62, half %tmp55, i64 4
  %tmp64 = insertelement <8 x half> %tmp63, half %tmp56, i64 5
  %tmp65 = insertelement <8 x half> %tmp64, half %tmp57, i64 6
  %tmp66 = insertelement <8 x half> %tmp65, half %tmp58, i64 7
  %tmp67 = bitcast <8 x half> %tmp66 to <8 x i16>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i16> %tmp67)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>)

declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32*, i32, i32, i1)

declare <64 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v64i16.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)

!igc.functions = !{}
