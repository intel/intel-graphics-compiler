;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformdg2 -simd-mode 8 -inputcs -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a0:0:32-n8:16:32-S32"

@ThreadGroupSize_X = constant i32 1
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; CHECK: .kernel "entry"
; CHECK: mov (M1_NM, 1) [[VECTOR:[A-z0-9]*]](0,0)<1> 0x3f800000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,1)<1> 0x40000000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,2)<1> 0x40400000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,3)<1> 0x40800000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,4)<1> 0x40a00000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,5)<1> 0x40c00000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,6)<1> 0x40e00000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](0,7)<1> 0x41000000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,0)<1> 0x41100000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,1)<1> 0x41200000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,2)<1> 0x41300000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,3)<1> 0x41400000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,4)<1> 0x41500000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,5)<1> 0x41600000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,6)<1> 0x41700000:f
; CHECK: mov (M1_NM, 1) [[VECTOR]](1,7)<1> 0x41800000:f
;   Check if we emit movs that fill all elements of STRUCT (32 elements / simd 8 == 4 MOVs).
; CHECK: mov (M1_NM, 8) [[STRUCT:[A-z0-9]*]](0,0)<1> [[VECTOR]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[STRUCT]](1,0)<1> [[VECTOR]](1,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[STRUCT]](2,0)<1> [[VECTOR]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[STRUCT]](3,0)<1> [[VECTOR]](1,0)<1;1,0>
;   Check if we emit copies from STRUCT to STRUCT_PHI_COPY
; CHECK: mov (M1_NM, 8) [[STRUCT_PHI_COPY:[A-z0-9]*]](0,0)<1> [[STRUCT]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[STRUCT_PHI_COPY]](1,0)<1> [[STRUCT]](1,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[STRUCT_PHI_COPY]](2,0)<1> [[STRUCT]](2,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[STRUCT_PHI_COPY]](3,0)<1> [[STRUCT]](3,0)<1;1,0>
;   Check if EXTRACT_VECTOR mov uses STRUCT_PHI_COPY variable.
; CHECK: mov (M1_NM, 8) [[EXTRACT_VECTOR:[A-z0-9]*]](0,0)<1> [[STRUCT_PHI_COPY]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 8) [[EXTRACT_VECTOR]](1,0)<1> [[STRUCT_PHI_COPY]](1,0)<1;1,0>
;   Check if we extract scalar and store it.
; CHECK: mov (M1_NM, 1) [[EXTRACT_SCALAR:[A-z0-9]*]](0,0)<1> [[EXTRACT_VECTOR]](0,3)<0;1,0>
; CHECK: lsc_store.ugm.wb.wb (M1_NM, 1)  {{.*}} [[EXTRACT_SCALAR]]

define void @entry(<8 x i32> %r0, float* %result) {
head:
  %vector0 = insertelement <16 x float> undef, float 1.0, i32 0
  %vector1 = insertelement <16 x float> %vector0, float 2.0, i32 1
  %vector2 = insertelement <16 x float> %vector1, float 3.0, i32 2
  %vector3 = insertelement <16 x float> %vector2, float 4.0, i32 3
  %vector4 = insertelement <16 x float> %vector3, float 5.0, i32 4
  %vector5 = insertelement <16 x float> %vector4, float 6.0, i32 5
  %vector6 = insertelement <16 x float> %vector5, float 7.0, i32 6
  %vector7 = insertelement <16 x float> %vector6, float 8.0, i32 7
  %vector8 = insertelement <16 x float> %vector7, float 9.0, i32 8
  %vector9 = insertelement <16 x float> %vector8, float 10.0, i32 9
  %vector10 = insertelement <16 x float> %vector9, float 11.0, i32 10
  %vector11 = insertelement <16 x float> %vector10, float 12.0, i32 11
  %vector12 = insertelement <16 x float> %vector11, float 13.0, i32 12
  %vector13 = insertelement <16 x float> %vector12, float 14.0, i32 13
  %vector14 = insertelement <16 x float> %vector13, float 15.0, i32 14
  %vector15 = insertelement <16 x float> %vector14, float 16.0, i32 15

  %vectorStruct0 = insertvalue { <16 x float>, <16 x float> } poison, <16 x float> %vector15, 0
  %vectorStruct1 = insertvalue { <16 x float>, <16 x float> } %vectorStruct0, <16 x float> %vector15, 1
  br label %body

body:
  %fromPhi = phi { <16 x float>, <16 x float> } [ %vectorStruct1, %head ]
  %extractVector0 = extractvalue { <16 x float>, <16 x float> } %fromPhi, 0
  %extractScalar3 = extractelement <16 x float> %extractVector0, i32 3
  store float %extractScalar3, float* %result, align 4
  ret void
}


!igc.functions = !{!1}

!1 = !{void (<8 x i32>, float*)* @entry, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
