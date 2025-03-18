;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers %s -S -o - --inputcs --regkey OverrideCsWalkOrderEnable=1,OverrideCsWalkOrder=0,OverrideCsTileLayoutEnable=1,OverrideCsTileLayout=0 -igc-constant-coalescing | FileCheck %s

; This test verifies bindless, uniform (through regkeys) ldraw merging with address calculation passing a phinode

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

@ThreadGroupSize_X = constant i32 16
@ThreadGroupSize_Y = constant i32 4
@ThreadGroupSize_Z = constant i32 2

define void @CSMain(<8 x i32> %r0, i8* %privateBase) #0 {
  %runtime1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %t1 = inttoptr i32 %runtime1 to <4 x float> addrspace(2621441)*
  %LocalID_Z = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 19)
  %BB0_end = mul i32 %LocalID_Z, 288
  br label %1
1:
  %phi = phi i32 [ %BB0_end, %0 ], [ %BB1_end, %1 ]
  %shl = shl i32 %phi, 4
;CHECK:  [[REGLDRAW0:%[0-9]+]] = call <32 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v32i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %shl, i32 4, i1 false)
  %ldraw0_0 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %shl, i32 4, i1 false)
;CHECK:  %ext0_0 = extractelement <32 x i32> [[REGLDRAW0]], i64 0
;CHECK:  %ext0_1 = extractelement <32 x i32> [[REGLDRAW0]], i64 1
;CHECK:  %ext0_2 = extractelement <32 x i32> [[REGLDRAW0]], i64 2
;CHECK:  %ext0_3 = extractelement <32 x i32> [[REGLDRAW0]], i64 3
  %ext0_0 = extractelement <4 x i32> %ldraw0_0, i64 0
  %ext0_1 = extractelement <4 x i32> %ldraw0_0, i64 1
  %ext0_2 = extractelement <4 x i32> %ldraw0_0, i64 2
  %ext0_3 = extractelement <4 x i32> %ldraw0_0, i64 3
  %or0_0 = or i32 %shl, 16
  %ldraw0_1 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_0, i32 4, i1 false)
;CHECK:  %ext0_4 = extractelement <32 x i32> [[REGLDRAW0]], i32 4
;CHECK:  %ext0_5 = extractelement <32 x i32> [[REGLDRAW0]], i32 5
;CHECK:  %ext0_6 = extractelement <32 x i32> [[REGLDRAW0]], i32 6
;CHECK:  %ext0_7 = extractelement <32 x i32> [[REGLDRAW0]], i32 7
  %ext0_4 = extractelement <4 x i32> %ldraw0_1, i64 0
  %ext0_5 = extractelement <4 x i32> %ldraw0_1, i64 1
  %ext0_6 = extractelement <4 x i32> %ldraw0_1, i64 2
  %ext0_7 = extractelement <4 x i32> %ldraw0_1, i64 3
  %or0_1 = or i32 %shl, 32
  %ldraw0_2 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_1, i32 4, i1 false)
;CHECK:  %ext0_8 = extractelement <32 x i32> [[REGLDRAW0]], i32 8
;CHECK:  %ext0_9 = extractelement <32 x i32> [[REGLDRAW0]], i32 9
;CHECK:  %ext0_10 = extractelement <32 x i32> [[REGLDRAW0]], i32 10
;CHECK:  %ext0_11 = extractelement <32 x i32> [[REGLDRAW0]], i32 11
  %ext0_8 = extractelement <4 x i32> %ldraw0_2, i64 0
  %ext0_9 = extractelement <4 x i32> %ldraw0_2, i64 1
  %ext0_10 = extractelement <4 x i32> %ldraw0_2, i64 2
  %ext0_11 = extractelement <4 x i32> %ldraw0_2, i64 3
  %or0_2 = or i32 %shl, 48
  %ldraw0_3 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_2, i32 4, i1 false)
;CHECK:  %ext0_12 = extractelement <32 x i32> [[REGLDRAW0]], i32 12
;CHECK:  %ext0_13 = extractelement <32 x i32> [[REGLDRAW0]], i32 13
;CHECK:  %ext0_14 = extractelement <32 x i32> [[REGLDRAW0]], i32 14
;CHECK:  %ext0_15 = extractelement <32 x i32> [[REGLDRAW0]], i32 15
  %ext0_12 = extractelement <4 x i32> %ldraw0_3, i64 0
  %ext0_13 = extractelement <4 x i32> %ldraw0_3, i64 1
  %ext0_14 = extractelement <4 x i32> %ldraw0_3, i64 2
  %ext0_15 = extractelement <4 x i32> %ldraw0_3, i64 3
  %or0_3 = or i32 %shl, 64
  %ldraw0_4 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_3, i32 4, i1 false)
;CHECK:  %ext0_16 = extractelement <32 x i32> [[REGLDRAW0]], i32 16
;CHECK:  %ext0_17 = extractelement <32 x i32> [[REGLDRAW0]], i32 17
;CHECK:  %ext0_18 = extractelement <32 x i32> [[REGLDRAW0]], i32 18
;CHECK:  %ext0_19 = extractelement <32 x i32> [[REGLDRAW0]], i32 19
  %ext0_16 = extractelement <4 x i32> %ldraw0_4, i64 0
  %ext0_17 = extractelement <4 x i32> %ldraw0_4, i64 1
  %ext0_18 = extractelement <4 x i32> %ldraw0_4, i64 2
  %ext0_19 = extractelement <4 x i32> %ldraw0_4, i64 3
  %or0_4 = or i32 %shl, 80
  %ldraw0_5 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_4, i32 4, i1 false)
;CHECK:  %ext0_20 = extractelement <32 x i32> [[REGLDRAW0]], i32 20
;CHECK:  %ext0_21 = extractelement <32 x i32> [[REGLDRAW0]], i32 21
;CHECK:  %ext0_22 = extractelement <32 x i32> [[REGLDRAW0]], i32 22
;CHECK:  %ext0_23 = extractelement <32 x i32> [[REGLDRAW0]], i32 23
  %ext0_20 = extractelement <4 x i32> %ldraw0_5, i64 0
  %ext0_21 = extractelement <4 x i32> %ldraw0_5, i64 1
  %ext0_22 = extractelement <4 x i32> %ldraw0_5, i64 2
  %ext0_23 = extractelement <4 x i32> %ldraw0_5, i64 3
  %or0_5 = or i32 %shl, 96
  %ldraw0_6 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_5, i32 4, i1 false)
;CHECK:  %ext0_24 = extractelement <32 x i32> [[REGLDRAW0]], i32 24
;CHECK:  %ext0_25 = extractelement <32 x i32> [[REGLDRAW0]], i32 25
;CHECK:  %ext0_26 = extractelement <32 x i32> [[REGLDRAW0]], i32 26
;CHECK:  %ext0_27 = extractelement <32 x i32> [[REGLDRAW0]], i32 27
  %ext0_24 = extractelement <4 x i32> %ldraw0_6, i64 0
  %ext0_25 = extractelement <4 x i32> %ldraw0_6, i64 1
  %ext0_26 = extractelement <4 x i32> %ldraw0_6, i64 2
  %ext0_27 = extractelement <4 x i32> %ldraw0_6, i64 3
  %or0_6 = or i32 %shl, 112
  %ldraw0_7 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or0_6, i32 4, i1 false)
;CHECK:  %ext0_28 = extractelement <32 x i32> [[REGLDRAW0]], i32 28
;CHECK:  %ext0_29 = extractelement <32 x i32> [[REGLDRAW0]], i32 29
;CHECK:  %ext0_30 = extractelement <32 x i32> [[REGLDRAW0]], i32 30
;CHECK:  %ext0_31 = extractelement <32 x i32> [[REGLDRAW0]], i32 31
  %ext0_28 = extractelement <4 x i32> %ldraw0_7, i64 0
  %ext0_29 = extractelement <4 x i32> %ldraw0_7, i64 1
  %ext0_30 = extractelement <4 x i32> %ldraw0_7, i64 2
  %ext0_31 = extractelement <4 x i32> %ldraw0_7, i64 3
  %or1_0 = or i32 %shl, 128
;CHECK:  [[REGLDRAW1:%[0-9]+]] = call <32 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v32i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %{{[0-9]+}}, i32 4, i1 false)
  %ldraw1_0 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_0, i32 4, i1 false)
;CHECK:  %ext1_0 = extractelement <32 x i32> [[REGLDRAW1]], i64 0
;CHECK:  %ext1_1 = extractelement <32 x i32> [[REGLDRAW1]], i64 1
;CHECK:  %ext1_2 = extractelement <32 x i32> [[REGLDRAW1]], i64 2
;CHECK:  %ext1_3 = extractelement <32 x i32> [[REGLDRAW1]], i64 3
  %ext1_0 = extractelement <4 x i32> %ldraw1_0, i64 0
  %ext1_1 = extractelement <4 x i32> %ldraw1_0, i64 1
  %ext1_2 = extractelement <4 x i32> %ldraw1_0, i64 2
  %ext1_3 = extractelement <4 x i32> %ldraw1_0, i64 3
  %or1_1 = or i32 %shl, 144
  %ldraw1_1 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_1, i32 4, i1 false)
;CHECK:  %ext1_4 = extractelement <32 x i32> [[REGLDRAW1]], i32 4
;CHECK:  %ext1_5 = extractelement <32 x i32> [[REGLDRAW1]], i32 5
;CHECK:  %ext1_6 = extractelement <32 x i32> [[REGLDRAW1]], i32 6
;CHECK:  %ext1_7 = extractelement <32 x i32> [[REGLDRAW1]], i32 7
  %ext1_4 = extractelement <4 x i32> %ldraw1_1, i64 0
  %ext1_5 = extractelement <4 x i32> %ldraw1_1, i64 1
  %ext1_6 = extractelement <4 x i32> %ldraw1_1, i64 2
  %ext1_7 = extractelement <4 x i32> %ldraw1_1, i64 3
  %or1_2 = or i32 %shl, 160
  %ldraw1_2 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_2, i32 4, i1 false)
;CHECK:  %ext1_8 = extractelement <32 x i32> [[REGLDRAW1]], i32 8
;CHECK:  %ext1_9 = extractelement <32 x i32> [[REGLDRAW1]], i32 9
;CHECK:  %ext1_10 = extractelement <32 x i32> [[REGLDRAW1]], i32 10
;CHECK:  %ext1_11 = extractelement <32 x i32> [[REGLDRAW1]], i32 11
  %ext1_8 = extractelement <4 x i32> %ldraw1_2, i64 0
  %ext1_9 = extractelement <4 x i32> %ldraw1_2, i64 1
  %ext1_10 = extractelement <4 x i32> %ldraw1_2, i64 2
  %ext1_11 = extractelement <4 x i32> %ldraw1_2, i64 3
  %or1_3 = or i32 %shl, 176
  %ldraw1_3 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_3, i32 4, i1 false)
;CHECK:  %ext1_12 = extractelement <32 x i32> [[REGLDRAW1]], i32 12
;CHECK:  %ext1_13 = extractelement <32 x i32> [[REGLDRAW1]], i32 13
;CHECK:  %ext1_14 = extractelement <32 x i32> [[REGLDRAW1]], i32 14
;CHECK:  %ext1_15 = extractelement <32 x i32> [[REGLDRAW1]], i32 15
  %ext1_12 = extractelement <4 x i32> %ldraw1_3, i64 0
  %ext1_13 = extractelement <4 x i32> %ldraw1_3, i64 1
  %ext1_14 = extractelement <4 x i32> %ldraw1_3, i64 2
  %ext1_15 = extractelement <4 x i32> %ldraw1_3, i64 3
  %or1_4 = or i32 %shl, 192
  %ldraw1_4 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_4, i32 4, i1 false)
;CHECK:  %ext1_16 = extractelement <32 x i32> [[REGLDRAW1]], i32 16
;CHECK:  %ext1_17 = extractelement <32 x i32> [[REGLDRAW1]], i32 17
;CHECK:  %ext1_18 = extractelement <32 x i32> [[REGLDRAW1]], i32 18
;CHECK:  %ext1_19 = extractelement <32 x i32> [[REGLDRAW1]], i32 19
  %ext1_16 = extractelement <4 x i32> %ldraw1_4, i64 0
  %ext1_17 = extractelement <4 x i32> %ldraw1_4, i64 1
  %ext1_18 = extractelement <4 x i32> %ldraw1_4, i64 2
  %ext1_19 = extractelement <4 x i32> %ldraw1_4, i64 3
  %or1_5 = or i32 %shl, 208
  %ldraw1_5 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_5, i32 4, i1 false)
;CHECK:  %ext1_20 = extractelement <32 x i32> [[REGLDRAW1]], i32 20
;CHECK:  %ext1_21 = extractelement <32 x i32> [[REGLDRAW1]], i32 21
;CHECK:  %ext1_22 = extractelement <32 x i32> [[REGLDRAW1]], i32 22
;CHECK:  %ext1_23 = extractelement <32 x i32> [[REGLDRAW1]], i32 23
  %ext1_20 = extractelement <4 x i32> %ldraw1_5, i64 0
  %ext1_21 = extractelement <4 x i32> %ldraw1_5, i64 1
  %ext1_22 = extractelement <4 x i32> %ldraw1_5, i64 2
  %ext1_23 = extractelement <4 x i32> %ldraw1_5, i64 3
  %or1_6 = or i32 %shl, 224
  %ldraw1_6 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_6, i32 4, i1 false)
;CHECK:  %ext1_24 = extractelement <32 x i32> [[REGLDRAW1]], i32 24
;CHECK:  %ext1_25 = extractelement <32 x i32> [[REGLDRAW1]], i32 25
;CHECK:  %ext1_26 = extractelement <32 x i32> [[REGLDRAW1]], i32 26
;CHECK:  %ext1_27 = extractelement <32 x i32> [[REGLDRAW1]], i32 27
  %ext1_24 = extractelement <4 x i32> %ldraw1_6, i64 0
  %ext1_25 = extractelement <4 x i32> %ldraw1_6, i64 1
  %ext1_26 = extractelement <4 x i32> %ldraw1_6, i64 2
  %ext1_27 = extractelement <4 x i32> %ldraw1_6, i64 3
  %or1_7 = or i32 %shl, 240
  %ldraw1_7 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)* %t1, i32 %or1_7, i32 4, i1 false)
;CHECK:  %ext1_28 = extractelement <32 x i32> [[REGLDRAW1]], i32 28
;CHECK:  %ext1_29 = extractelement <32 x i32> [[REGLDRAW1]], i32 29
;CHECK:  %ext1_30 = extractelement <32 x i32> [[REGLDRAW1]], i32 30
;CHECK:  %ext1_31 = extractelement <32 x i32> [[REGLDRAW1]], i32 31
  %ext1_28 = extractelement <4 x i32> %ldraw1_7, i64 0
  %ext1_29 = extractelement <4 x i32> %ldraw1_7, i64 1
  %ext1_30 = extractelement <4 x i32> %ldraw1_7, i64 2
  %ext1_31 = extractelement <4 x i32> %ldraw1_7, i64 3
  %BB1_end = add nsw i32 %phi, 144
  br label %1
}

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #7

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #7

; Function Attrs: argmemonly nounwind readonly
declare <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621441v4f32(<4 x float> addrspace(2621441)*, i32, i32, i1) #7

; Function Attrs: argmemonly nounwind readonly
declare <32 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v32i32.p2621441v4f32(<4 x float> addrspace(2621441)*, i32, i32, i1) #7

attributes #7 = { argmemonly nounwind readonly }

!igc.functions = !{!19}

!19 = !{void (<8 x i32>, i8*)* @CSMain, !20}
!20 = !{!21, !22}
!21 = !{!"function_type", i32 0}
!22 = !{!"implicit_arg_desc", !23, !24}
!23 = !{i32 0}
!24 = !{i32 13}
