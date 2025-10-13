;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -igc-emit-visa %s -dx12 -inputcs -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

@ThreadGroupSize_X = constant i32 1
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 16

; Function Attrs: null_pointer_is_valid
define void @CSMain(i32 %runtime_value_0, i32 %runtime_value_1, i32 %runtime_value_2) #0 {
  %src = inttoptr i32 %runtime_value_0 to <4 x float> addrspace(2490368)*
  %dst = inttoptr i32 %runtime_value_2 to <4 x float> addrspace(2490369)*
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %varOffset = add i32 %runtime_value_1, %lane32
; CHECK: lsc_load.ugm (M1, 32) read_0:d32x4 bss(runtime_value_0)[varOffset]:a32
  %read_0 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %varOffset, i32 4, i1 false)
  %ext0_0 = extractelement <4 x i32> %read_0, i32 0
  %ext0_1 = extractelement <4 x i32> %read_0, i32 1
  %ext0_2 = extractelement <4 x i32> %read_0, i32 2
  %ext0_3 = extractelement <4 x i32> %read_0, i32 3
  %addr_1 = add i32 %varOffset, 256
; CHECK: lsc_load.ugm (M1, 32) read_1:d32x4 bss(runtime_value_0)[varOffset+0x100]:a32
  %read_1 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %addr_1, i32 4, i1 false)
  %ext1_0 = extractelement <4 x i32> %read_1, i32 0
  %ext1_1 = extractelement <4 x i32> %read_1, i32 1
  %ext1_2 = extractelement <4 x i32> %read_1, i32 2
  %ext1_3 = extractelement <4 x i32> %read_1, i32 3
  %addr_2 = add i32 %varOffset, 512
; CHECK: lsc_load.ugm (M1, 32) read_2:d32x4 bss(runtime_value_0)[varOffset+0x200]:a32
  %read_2 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %addr_2, i32 4, i1 false)
  %ext2_0 = extractelement <4 x i32> %read_2, i32 0
  %ext2_1 = extractelement <4 x i32> %read_2, i32 1
  %ext2_2 = extractelement <4 x i32> %read_2, i32 2
  %ext2_3 = extractelement <4 x i32> %read_2, i32 3
  %addr_3 = add i32 %varOffset, 768
; CHECK: lsc_load.ugm (M1, 32) read_3:d32x4 bss(runtime_value_0)[varOffset+0x300]:a32
  %read_3 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %addr_3, i32 4, i1 false)
  %ext3_0 = extractelement <4 x i32> %read_3, i32 0
  %ext3_1 = extractelement <4 x i32> %read_3, i32 1
  %ext3_2 = extractelement <4 x i32> %read_3, i32 2
  %ext3_3 = extractelement <4 x i32> %read_3, i32 3
  %add0_0_1 = add i32 %ext0_0, %ext1_0
  %add0_2_3 = add i32 %ext2_0, %ext3_0
  %add0 = add i32 %add0_0_1, %add0_2_3
  %add1_0_1 = add i32 %ext0_1, %ext1_1
  %add1_2_3 = add i32 %ext2_1, %ext3_1
  %add1 = add i32 %add1_0_1, %add1_2_3
  %add2_0_1 = add i32 %ext0_0, %ext1_0
  %add2_2_3 = add i32 %ext2_0, %ext3_0
  %add2= add i32 %add2_0_1, %add2_2_3
  %add3_0_1 = add i32 %ext0_0, %ext1_0
  %add3_2_3 = add i32 %ext2_0, %ext3_0
  %add3 = add i32 %add3_0_1, %add3_2_3
  %res0 = insertelement <4 x i32> undef, i32 %add0, i64 0
  %res1 = insertelement <4 x i32> %res0, i32 %add1, i64 1
  %res2 = insertelement <4 x i32> %res1, i32 %add2, i64 2
  %res3 = insertelement <4 x i32> %res2, i32 %add3, i64 3
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490369v4f32.v4i32(<4 x float> addrspace(2490369)* %dst, i32 0,  <4 x i32> %res3, i32 4, i1 false)
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2490368v4f32(<4 x float> addrspace(2490368)*, i32, i32, i1) #1

declare void @llvm.genx.GenISA.storerawvector.indexed.p2490369v4f32.v4i32(<4 x float> addrspace(2490369)*, i32, <4 x i32>, i32, i1) #2

declare i16 @llvm.genx.GenISA.simdLaneId() #3

attributes #0 = { null_pointer_is_valid }
attributes #1 = { argmemonly nounwind readonly }
attributes #2 = { argmemonly nounwind writeonly }
attributes #3 = { nounwind readnone }

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i32, i32, i32)* @CSMain, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i32, i32, i32)* @CSMain}
!6 = !{!"FuncMDValue[0]"}