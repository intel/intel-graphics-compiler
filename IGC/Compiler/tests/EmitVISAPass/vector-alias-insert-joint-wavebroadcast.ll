;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=2 | FileCheck %s

; emitJointWaveBroadcast maps the JointWaveBroadcast result to a uniform alias
; of its source after the VRA alias variables are created. When the result is
; inserted into a larger vector (concatenated with constants), it must not be
; VRA-aliased into that vector, otherwise the vector's first half is never
; written. Lanes are 2..9, so the first half must be copied from src(0,2).

; CHECK-LABEL: .kernel "test"
; CHECK:       lsc_load.ugm (M1, 16) [[SRC:[A-Za-z0-9_]+]]:d32
; CHECK-NEXT:  mov (M1_NM, 1) [[V:[A-Za-z0-9_]+]](0,0)<1> [[SRC]](0,2)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,1)<1> [[SRC]](0,3)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,2)<1> [[SRC]](0,4)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,3)<1> [[SRC]](0,5)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,4)<1> [[SRC]](0,6)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,5)<1> [[SRC]](0,7)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,6)<1> [[SRC]](0,8)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,7)<1> [[SRC]](0,9)<0;1,0>
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,8)<1> 0x3f800000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,9)<1> 0x40000000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,10)<1> 0x40400000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,11)<1> 0x40800000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,12)<1> 0x40a00000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,13)<1> 0x40c00000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,14)<1> 0x40e00000:f
; CHECK-NEXT:  mov (M1_NM, 1) [[V]](0,15)<1> 0x41000000:f

define spir_kernel void @test() {
entry:
  %lid = call i16 @llvm.genx.GenISA.simdLaneId()
  %lid32 = zext i16 %lid to i64
  %inp = getelementptr float, ptr addrspace(1) null, i64 %lid32
  %src = load float, ptr addrspace(1) %inp, align 4
  %jwb = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>, i32 0)
  %e0 = extractelement <8 x float> %jwb, i32 0
  %v0 = insertelement <16 x float> undef, float %e0, i32 0
  %e1 = extractelement <8 x float> %jwb, i32 1
  %v1 = insertelement <16 x float> %v0, float %e1, i32 1
  %e2 = extractelement <8 x float> %jwb, i32 2
  %v2 = insertelement <16 x float> %v1, float %e2, i32 2
  %e3 = extractelement <8 x float> %jwb, i32 3
  %v3 = insertelement <16 x float> %v2, float %e3, i32 3
  %e4 = extractelement <8 x float> %jwb, i32 4
  %v4 = insertelement <16 x float> %v3, float %e4, i32 4
  %e5 = extractelement <8 x float> %jwb, i32 5
  %v5 = insertelement <16 x float> %v4, float %e5, i32 5
  %e6 = extractelement <8 x float> %jwb, i32 6
  %v6 = insertelement <16 x float> %v5, float %e6, i32 6
  %e7 = extractelement <8 x float> %jwb, i32 7
  %v7 = insertelement <16 x float> %v6, float %e7, i32 7
  %v8 = insertelement <16 x float> %v7, float 1.000000e+00, i32 8
  %v9 = insertelement <16 x float> %v8, float 2.000000e+00, i32 9
  %v10 = insertelement <16 x float> %v9, float 3.000000e+00, i32 10
  %v11 = insertelement <16 x float> %v10, float 4.000000e+00, i32 11
  %v12 = insertelement <16 x float> %v11, float 5.000000e+00, i32 12
  %v13 = insertelement <16 x float> %v12, float 6.000000e+00, i32 13
  %v14 = insertelement <16 x float> %v13, float 7.000000e+00, i32 14
  %v15 = insertelement <16 x float> %v14, float 8.000000e+00, i32 15
  store <16 x float> %v15, ptr addrspace(1) null, align 64
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float, <8 x i32>, i32)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!IGCMetadata = !{!3}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"requiredSubGroupSize", i32 16}
