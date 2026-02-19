;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s


 ; Given  store i64* v0,  i64** p
 ;        store i64* v1,  i64** p+1
 ;   combined into
 ;        v = ((i64)v0, (i64)v1)
 ;        store <2xi64> v, p
 ;
 ; CHECK-LABEL: define spir_kernel void @test_two_pointers
 ; CHECK: [[TMP1:%.*]] = insertelement <2 x i64> undef, i64 %{{.*}}, i32 0
 ; CHECK: [[TMP2:%.*]] = insertelement <2 x i64> [[TMP1]], i64 %{{.*}}, i32 1
 ; CHECK: [[TMP3:%.*]] = bitcast i64 addrspace(1)* addrspace(1)* %{{.*}} to <2 x i64> addrspace(1)*
 ; CHECK: store <2 x i64> [[TMP2]], <2 x i64> addrspace(1)* [[TMP3]], align 8
 ; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_two_pointers(i64 addrspace(1)* %d, i64 addrspace(1)* %s, i16 %localIdX) {
entry:
  %P = bitcast i64 addrspace(1)* %d to i64 addrspace(1)* addrspace(1)*
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx = getelementptr inbounds i64 addrspace(1)*, i64 addrspace(1)* addrspace(1)* %P, i64 %conv.i.i
  store i64 addrspace(1)* %d, i64 addrspace(1)* addrspace(1)* %arrayidx, align 8
  %add = add nuw nsw i64 %conv.i.i, 1
  %arrayidx1 = getelementptr inbounds i64 addrspace(1)*, i64 addrspace(1)* addrspace(1)* %P, i64 %add
  store i64 addrspace(1)* %s, i64 addrspace(1)* addrspace(1)* %arrayidx1, align 8
  ret void
}
