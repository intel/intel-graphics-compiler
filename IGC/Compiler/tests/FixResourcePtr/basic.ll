;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --igc-fix-resource-ptr -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --igc-fix-resource-ptr -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; FixResourcePtr
; ------------------------------------------------

define spir_kernel void @test_direct(i32 %src) {
; CHECK-LABEL: @test_direct(
; CHECK-TYPED:    [[TMP1:%.*]] = call i32 addrspace(131072)* @llvm.genx.GenISA.GetBufferPtr.p131072i32(i32 13, i32 1)
; CHECK-OPAQUE:    [[TMP1:%.*]] = call ptr addrspace(131072) @llvm.genx.GenISA.GetBufferPtr.p131072(i32 13, i32 1)
; CHECK-TYPED:    [[TMP2:%.*]] = load <2 x i16>, <2 x i16> addrspace(131085)* null
; CHECK-OPAQUE:    [[TMP2:%.*]] = load <2 x i16>, ptr addrspace(131085) null
; CHECK-TYPED:    store <2 x i16> [[TMP2]], <2 x i16> addrspace(131085)* null
; CHECK-OPAQUE:    store <2 x i16> [[TMP2]], ptr addrspace(131085) null
; CHECK-TYPED:    store i32 [[SRC:%.*]], i32 addrspace(131085)* inttoptr (i64 16 to i32 addrspace(131085)*)
; CHECK-OPAQUE:    store i32 [[SRC:%.*]], ptr addrspace(131085) inttoptr (i64 16 to ptr addrspace(131085))
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
; CHECK-TYPED:    [[TMP1:%.*]] = call i32 addrspace(2752512)* @llvm.genx.GenISA.GetBufferPtr.p2752512i32(i32 13, i32 1)
; CHECK-OPAQUE:    [[TMP1:%.*]] = call ptr addrspace(2752512) @llvm.genx.GenISA.GetBufferPtr.p2752512(i32 13, i32 1)
; CHECK-TYPED:    [[TMP2:%.*]] = call <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.v2i16.p2752512i32(i32 addrspace(2752512)* [[TMP1]], i32 0, i32 {{2|4}}, i1 false)
; CHECK-OPAQUE:    [[TMP2:%.*]] = call <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.v2i16.p2752512(ptr addrspace(2752512) [[TMP1]], i32 0, i32 {{2|4}}, i1 false)
; CHECK-TYPED:    call void @llvm.genx.GenISA.storerawvector.indexed.p2752512i32.v2i16(i32 addrspace(2752512)* [[TMP1]], i32 0, <2 x i16> [[TMP2]], i32 16, i1 false)
; CHECK-OPAQUE:    call void @llvm.genx.GenISA.storerawvector.indexed.p2752512.v2i16(ptr addrspace(2752512) [[TMP1]], i32 0, <2 x i16> [[TMP2]], i32 16, i1 false)
; CHECK-TYPED:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2752512i32(i32 addrspace(2752512)* [[TMP1]], i32 16, i32 16, i1 false)
; CHECK-OPAQUE:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2752512(ptr addrspace(2752512) [[TMP1]], i32 16, i32 16, i1 false)
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to float
; CHECK-TYPED:    call void @llvm.genx.GenISA.storeraw.indexed.p2752512i32.f32(i32 addrspace(2752512)* [[TMP1]], i32 16, float [[TMP4]], i32 {{2|4}}, i1 false)
; CHECK-OPAQUE:    call void @llvm.genx.GenISA.storeraw.indexed.p2752512.f32(ptr addrspace(2752512) [[TMP1]], i32 16, float [[TMP4]], i32 {{2|4}}, i1 false)
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

