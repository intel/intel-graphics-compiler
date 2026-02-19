;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-fix-resource-ptr -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; FixResourcePtr
; ------------------------------------------------

define spir_kernel void @test_direct(i32 %src) {
; CHECK-LABEL: @test_direct(
; CHECK:    [[TMP1:%.*]] = call i32 addrspace(131072)* @llvm.genx.GenISA.GetBufferPtr.p131072i32(i32 13, i32 1)
; CHECK:    [[TMP2:%.*]] = load <2 x i16>, <2 x i16> addrspace(131085)* null
; CHECK:    store <2 x i16> [[TMP2]], <2 x i16> addrspace(131085)* null
; CHECK:    store i32 [[SRC:%.*]], i32 addrspace(131085)* inttoptr (i64 16 to i32 addrspace(131085)*)
; CHECK:    ret void
;
  %1 = call i32 addrspace(131072)* @llvm.genx.GenISA.GetBufferPtr.p131072i32(i32 13, i32 1)
  %2 = bitcast i32 addrspace(131072)* %1 to <2 x i16> addrspace(131072)*
  %3 = load <2 x i16>, <2 x i16> addrspace(131072)* %2
  store <2 x i16> %3, <2 x i16> addrspace(131072)* %2
  %4 = getelementptr inbounds i32, i32 addrspace(131072)* %1, i64 4
  %5 = load i32, i32 addrspace(131072)* %4
  store i32 %src, i32 addrspace(131072)* %4
  ret void
}

define spir_kernel void @test_indirect(i32 %src) {
; CHECK-LABEL: @test_indirect(
; CHECK:    [[TMP1:%.*]] = call i32 addrspace(2752512)* @llvm.genx.GenISA.GetBufferPtr.p2752512i32(i32 13, i32 1)
; CHECK:    [[TMP2:%.*]] = call <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.v2i16.p2752512i32(i32 addrspace(2752512)* [[TMP1]], i32 0, i32 {{2|4}}, i1 false)
; CHECK:    call void @llvm.genx.GenISA.storerawvector.indexed.p2752512i32.v2i16(i32 addrspace(2752512)* [[TMP1]], i32 0, <2 x i16> [[TMP2]], i32 16, i1 false)
; CHECK:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2752512i32(i32 addrspace(2752512)* [[TMP1]], i32 16, i32 16, i1 false)
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to float
; CHECK:    call void @llvm.genx.GenISA.storeraw.indexed.p2752512i32.f32(i32 addrspace(2752512)* [[TMP1]], i32 16, float [[TMP4]], i32 {{2|4}}, i1 false)
; CHECK:    ret void
;
  %1 = call i32 addrspace(2752512)* @llvm.genx.GenISA.GetBufferPtr.p2752512i32(i32 13, i32 1)
  %2 = bitcast i32 addrspace(2752512)* %1 to <2 x i16> addrspace(2752512)*
  %3 = load <2 x i16>, <2 x i16> addrspace(2752512)* %2
  store <2 x i16> %3, <2 x i16> addrspace(2752512)* %2, align 16
  %4 = getelementptr inbounds i32, i32 addrspace(2752512)* %1, i32 4
  %5 = load i32, i32 addrspace(2752512)* %4, align 16
  store i32 %5, i32 addrspace(2752512)* %4
  ret void
}

declare i32 addrspace(131072)* @llvm.genx.GenISA.GetBufferPtr.p131072i32(i32, i32)
declare i32 addrspace(2752512)* @llvm.genx.GenISA.GetBufferPtr.p2752512i32(i32, i32)

