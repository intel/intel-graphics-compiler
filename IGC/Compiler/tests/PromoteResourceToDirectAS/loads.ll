;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-promote-resources-to-direct-addrspace -dce -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteResourceToDirectAS
; ------------------------------------------------

define spir_kernel void @test_load(i32 %src) {
; CHECK-LABEL: @test_load(
; CHECK:    [[TMP3:%.*]] = getelementptr inbounds float, float addrspace(3)* null, i64 2
; CHECK:    [[TMP4:%.*]] = bitcast float addrspace(3)* [[TMP3]] to i32 addrspace(3)*
; CHECK:    [[TMP5:%.*]] = load i32, i32 addrspace(196608)* null
; CHECK:    store i32 [[TMP5]], i32 addrspace(3)* [[TMP4]]
; CHECK:    ret void
;
  %1 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 0, i32 2)
  %2 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 1, i32 3)
  %3 = addrspacecast i32 addrspace(196608)* %1 to i32 addrspace(1)*
  %4 = addrspacecast i32 addrspace(196608)* %2 to float addrspace(2)*
  %5 = getelementptr inbounds float, float addrspace(2)* %4, i64 2
  %6 = bitcast float addrspace(2)* %5 to i32 addrspace(2)*
  %7 = load i32, i32 addrspace(1)* %3
  store i32 %7, i32 addrspace(2)* %6
  ret void
}

define spir_kernel void @test_load_intrinsic(i32 %src) {
; CHECK-LABEL: @test_load_intrinsic(
; CHECK:    [[TMP3:%.*]] = load float, float addrspace(196608)* inttoptr (i32 4 to float addrspace(196608)*), align 4
; CHECK:    store float [[TMP3]], float addrspace(3)* inttoptr (i32 4 to float addrspace(3)*), align 8
; CHECK:    ret void
;
  %1 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 0, i32 2)
  %2 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 1, i32 3)
  %3 = addrspacecast i32 addrspace(196608)* %1 to float addrspace(1)*
  %4 = addrspacecast i32 addrspace(196608)* %2 to float addrspace(2)*
  %5 = call float @llvm.genx.GenISA.ldraw_indexed.p1f32(float addrspace(1)* %3, i32 4, i32 4, i1 false)
  call void @llvm.genx.GenISA.storeraw_indexed.p2f32(float addrspace(2)* %4, i32 4, float %5, i32 8, i1 false)
  ret void
}


declare float @llvm.genx.GenISA.ldraw_indexed.p1f32(float addrspace(1)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storeraw_indexed.p2f32(float addrspace(2)*, i32, float, i32, i1)
declare i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32, i32)
